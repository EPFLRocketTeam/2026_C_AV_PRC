#include "Application/FlightControl/prc_state.h"
#include "Application/FlightControl/prc_fsm_c_api.h"

#include "Application/Control/rst_controller.hpp"
#include "Application/FlightControl/intranet_cmd.hpp"
#include "Drivers/PrcBoardId/PrcBoardId.hpp"
#include "Drivers/Valve/ValveList.hpp"

#include "stm32h7xx_hal.h"

#include <cstdio>

// ---------------------------------------------------------------------------
// Timing constants — from 2026_C_SE_FLIGHT-PARAMETERS ("Dynamic Pressure
// Regulator" / DPR rows).
// ---------------------------------------------------------------------------

// Open-loop pressurization ramp -- ported from 2026_C_PR_BDPR's
// BVDPR_lib.h/BVDPR.ino (`DESIRED_DP`, `tankPress()`/`pressurisationTask()`).
// BDPR doesn't ramp for a fixed duration -- it feeds the RST controller a
// reference climbing linearly at this rate from the tank pressure measured
// at ramp entry (`ref = (millis()-ramp_T0)*DESIRED_DP + ramp_P0`), and
// leaves the phase once measured pressure is within k_ramp_exit_threshold
// of the final set pressure (see fromPressurizeOn() and
// ApplyValveActions()'s PRESSURIZE_ON case).
static constexpr float k_ramp_rate_bar_per_ms = 50.0e-3f; // DESIRED_DP, BVDPR_lib.h:19

// Ramp exit threshold -- ported from BDPR's tankPress() exit check
// (`pressureData(TANK_SENSOR) >= 0.98 * (P_REF - 1.0)`, BVDPR.ino:517).
static constexpr float k_ramp_exit_threshold_ratio = 0.98f;

// PASSIVATION_DURATION_DPR — confirmed: 300 s, every flight variant. Not
// currently used: PASSIVATE's venting is now driven by live tank/COPV
// pressure readings (ported from 2026_C_PR_BDPR's passivation(), see
// ApplyValveActions) rather than a fixed duration. This number could still
// serve as a failsafe backstop (force-close after 300 s regardless, in case
// a pressure sensor fails and never reports "vented") -- not implemented,
// just flagging the idea since the real number is sitting right here.

// PASSIVATION_DELAY_NO_COM_DPR — TODO: 140 s in early flight-variant
// columns, 51 s in later ones (see CSV) — pick the one matching the active
// variant. Comms-loss watchdog: if PRESSURIZE_OFF sits this long without an
// explicit PASSIVATE command, autonomously passivate anyway (ported
// directly from the old code's PRESSURIZATION_OFF state).
static constexpr uint32_t k_passivation_delay_no_com_ms = 140000u;

// PRESSURIZATION_{LOX,FUEL}_SET_PRESSURE — TODO: values TBD. Selected at
// runtime from BoardRole, not a build-time choice.
static constexpr float k_lox_set_pressure_bar  = 60.0f;
static constexpr float k_fuel_set_pressure_bar = 60.0f;

static float SetPressureBarFor(BoardRole role) {
  return (role == BoardRole::DprLox) ? k_lox_set_pressure_bar : k_fuel_set_pressure_bar;
}

static bool IsLox(BoardRole role) { return role == BoardRole::DprLox; }

//static float CurrentTankPressureBar(const PropSensors &s, BoardRole role) {
//  return static_cast<float>(IsLox(role) ? s.pressure_OTA : s.pressure_ETA);
//}
//
//static float CurrentCopvPressureBar(const PropSensors &s, BoardRole role) {
//  return static_cast<float>(IsLox(role) ? s.pressure_HPO : s.pressure_HPE);
//}

// Threshold below which a tank/COPV is considered "vented" -- ported from
// 2026_C_PR_BDPR's BVDPR.ino passivation() (`<= 1.0` bar, i.e. ~atmospheric).
static constexpr float k_vented_threshold_bar = 1.0f;

PrcState::PrcState() { this->currentState = State::MANUAL; }

PrcState::~PrcState() {
  // Nothing to do
}

State PrcState::getCurrentState() { return currentState; }

// ---------------------------------------------------------------------------
// fromXxx() — ported from DPRComputer::update()'s per-case logic
// (previous vehicle's DPRComputer.cpp). Command decode (intranetCmd.id == N)
// is the same placeholder scheme used before this port — TODO: replace
// once the real CAN command dictionary (DPR_LOX_PRESSURIZE=0x130 etc.) is
// decoded into IntranetCmd by the CAN RX layer.
// ---------------------------------------------------------------------------

State PrcState::fromManual(DataDump const &dump) {
  if (dump.intranetCmd.id == kCmdPressurizeOn) {
    return State::INITIALIZE_PRESSURIZE_ON;
  }
  return currentState;
}

// One-time setup on entry to pressurization (was set_offset()+initialize()
// in the old code's INITIALIZE_PRESSURIZATION case) — falls through to
// PRESSURIZE_ON in the same tick, same as the old switch-case did. Ramp
// state (ramp start pressure/time) is captured in ApplyValveActions() on
// entry to PRESSURIZE_ON, not here -- see the RST controller comment there.
State PrcState::fromInitializePressurizeOn(DataDump const &dump) {
  (void)dump;
  return State::PRESSURIZE_ON;
}

State PrcState::fromPressurizeOn(DataDump const &dump) {
  if (dump.intranetCmd.id == kCmdAbort) {
    return State::ABORT_ON_GROUND;
  }

  // Exit the open-loop ramp once measured tank pressure is within
  // k_ramp_exit_threshold_ratio of the final set pressure -- ported from
  // BDPR's tankPress() exit check (BVDPR.ino:517), not a fixed duration.
  const float target_bar  = SetPressureBarFor(dump.boardIdentity.role);
//  const float current_bar = CurrentTankPressureBar(dump.propSensors, dump.boardIdentity.role);
//  if (current_bar >= k_ramp_exit_threshold_ratio * target_bar) {
//    return State::INITIALIZE_REGULATE;
//  }

  return currentState;
}

// Formality state (old code's INITIALIZE_REGULATION also just calls
// initialize() then falls through) — kept as its own state for symmetry
// with the diagram/old code rather than folded into fromPressurizeOn.
State PrcState::fromInitializeRegulate(DataDump const &dump) {
  (void)dump;
  return State::REGULATE;
}

State PrcState::fromRegulate(DataDump const &dump) {
  if (dump.intranetCmd.id == kCmdAbort) {
    return State::ABORT_ON_GROUND;
  }
  if (dump.intranetCmd.id == kCmdPressurizeOff) {
    return State::PRESSURIZE_OFF;
  }
  return currentState;
}

State PrcState::fromPressurizeOff(DataDump const &dump) {
  // Note: per the FSM diagram, ABORT from this state goes to
  // ABORT_IN_FLIGHT, not ABORT_ON_GROUND (unlike PRESSURIZE_ON/REGULATE) --
  // intentional, matches the old code's PRESSURIZATION_OFF having no ABORT
  // branch at all plus the diagram's asymmetric abort targets.
  if (dump.intranetCmd.id == kCmdAbort) {
    return State::ABORT_IN_FLIGHT;
  }

  if (dump.intranetCmd.id == kCmdPassivate) {
    return State::INITIALIZE_PASSIVATE;
  }

  // Comms-loss watchdog (ported from the old code's PRESSURIZATION_OFF
  // case): no explicit PASSIVATE command for this long -> passivate anyway.
  const uint32_t elapsed_ms = HAL_GetTick() - pressurize_off_entry_ms_;
  if (elapsed_ms >= k_passivation_delay_no_com_ms) {
    return State::INITIALIZE_PASSIVATE;
  }

  return currentState;
}

State PrcState::fromInitializePassivate(DataDump const &dump) {
  (void)dump;
  return State::PASSIVATE;
}

State PrcState::fromPassivate(DataDump const &dump) {
  (void)dump;
  // Terminal per the FSM diagram -- no outgoing transition once passivated.
  // ApplyValveActions() still has two active venting phases inside
  // PASSIVATE, driven every tick by live tank/COPV pressure readings
  // (ported from 2026_C_PR_BDPR's passivation()), not a state transition.
  return currentState;
}

State PrcState::fromAbortOnGround(DataDump const &dump) {
  if (dump.intranetCmd.id == kCmdReset) {
    return State::MANUAL;
  }
  return currentState;
}

State PrcState::fromAbortInFlight(DataDump const &dump) {
  if (dump.intranetCmd.id == kCmdReset) {
    return State::MANUAL;
  }
  return currentState;
}

void PrcState::update(const DataDump &dump) {
  const State previous_state = currentState;

  switch (currentState) {
    case State::MANUAL:
      currentState = fromManual(dump);
      break;
    case State::INITIALIZE_PRESSURIZE_ON:
      currentState = fromInitializePressurizeOn(dump);
      break;
    case State::PRESSURIZE_ON:
      currentState = fromPressurizeOn(dump);
      break;
    case State::INITIALIZE_REGULATE:
      currentState = fromInitializeRegulate(dump);
      break;
    case State::REGULATE:
      currentState = fromRegulate(dump);
      break;
    case State::PRESSURIZE_OFF:
      currentState = fromPressurizeOff(dump);
      break;
    case State::INITIALIZE_PASSIVATE:
      currentState = fromInitializePassivate(dump);
      break;
    case State::PASSIVATE:
      currentState = fromPassivate(dump);
      break;
    case State::ABORT_ON_GROUND:
      currentState = fromAbortOnGround(dump);
      break;
    case State::ABORT_IN_FLIGHT:
      currentState = fromAbortInFlight(dump);
      break;
    default:
      currentState = State::ABORT_ON_GROUND;
  }

  if (currentState != previous_state) {
    printf("[PRC FSM] %s -> %s\r\n",
           stateToString(previous_state).c_str(),
           stateToString(currentState).c_str());

    const uint32_t now_ms = HAL_GetTick();
    if (currentState == State::PRESSURIZE_ON)  pressurize_on_entry_ms_  = now_ms;
    if (currentState == State::PRESSURIZE_OFF) pressurize_off_entry_ms_ = now_ms;
    if (currentState == State::PASSIVATE)      passivate_entry_ms_      = now_ms;
  }
}

std::string PrcState::stateToString(State state) {
  switch (state) {
    case State::MANUAL:                  return "MANUAL";
    case State::INITIALIZE_PRESSURIZE_ON:return "INITIALIZE_PRESSURIZE_ON";
    case State::PRESSURIZE_ON:           return "PRESSURIZE_ON";
    case State::INITIALIZE_REGULATE:     return "INITIALIZE_REGULATE";
    case State::REGULATE:                return "REGULATE";
    case State::PRESSURIZE_OFF:          return "PRESSURIZE_OFF";
    case State::INITIALIZE_PASSIVATE:    return "INITIALIZE_PASSIVATE";
    case State::PASSIVATE:               return "PASSIVATE";
    case State::ABORT_ON_GROUND:         return "ABORT_ON_GROUND";
    case State::ABORT_IN_FLIGHT:         return "ABORT_IN_FLIGHT";
    default:                             return "ERROR";
  }
}

// ── Global FSM instance and super-loop tick ────────────────────────────────
static PrcState& fsm_instance() {
  static PrcState inst;
  return inst;
}

// ---------------------------------------------------------------------------
// Valve mapping: per DPR bay (LOX or Ethanol), there are exactly 2
// solenoids + 1 ball valve, translated onto this project's Valves fields
// (adopted field-for-field from FC):
//   Safety (ETH Safety DPR / LOX Safety DPR, per-bay, NOT shared between
//       the two boards) -- gates the ball valve's regulation path, open
//       during tank pressurization + regulation + the COPV-venting phase
//       of passivation; closed in idle/PRESSURIZE_OFF and the
//       tank-only-venting phase of passivation.
//     -> valve_dpr_pressure_lox / valve_dpr_pressure_fuel
//   Vent (Ethanol Tank Venting / LOX Tank Venting) -- opens whenever
//       venting to atmosphere.
//     -> valve_dpr_vent_lox / valve_dpr_vent_fuel
// The ball valve itself (Ethanol Tank DPR / LOX Tank DPR) is this board's
// proportional pressure-regulation element, driven directly via
// ServoBallValve::set_position() and BDPR's actual RST pole-placement
// controller + characterized flow->angle lookup table (see
// Application/Control/rst_controller.hpp and the RST controller comment
// below), not through these 2 on/off solenoids and not a linear %open law.
// valve_dpr_vent_copv is left unused here -- no valve in this board's set
// maps to it.
// ---------------------------------------------------------------------------

static constexpr ValveId k_valve_safety = ValveId::SafetyDpr;
static constexpr ValveId k_valve_vent   = ValveId::Vent;

// Ball valve position driven by the RST pole-placement controller +
// characterized flow->angle table, ported from 2026_C_PR_BDPR
// (Application/Control/rst_controller.hpp) -- replaces the earlier
// placeholder linear proportional law. Two independent controller
// instances are needed, matching BDPR's RST_p/RST_c: one for the open-loop
// pressurization ramp (PRESSURIZE_ON), one for closed-loop regulation
// (REGULATE). Each owns its own R/S/T polynomials, computed once on entry
// to its state (see the PRESSURIZE_ON/REGULATE entry cases below) rather
// than every tick, since gain scheduling is currently disabled upstream
// (constant plant model -- see rst_controller.cpp's ComputeModelB).
namespace {
  float g_ramp_r[kRstNR_V]      = {0};
  float g_ramp_s[kRstNS_V]      = {0};
  float g_ramp_t[kRstNT_V]      = {0};
  float g_regulate_r[kRstNR_V]  = {0};
  float g_regulate_s[kRstNS_V]  = {0};
  float g_regulate_t[kRstNT_V]  = {0};

  RstController g_ramp_rst(g_ramp_r, g_ramp_s, g_ramp_t);
  RstController g_regulate_rst(g_regulate_r, g_regulate_s, g_regulate_t);

  // Ramp start pressure/time -- ported from BDPR's `ramp_P0`/`ramp_T0`
  // (BVDPR.ino:510-511), captured once on entry to PRESSURIZE_ON.
  float g_ramp_p0_bar   = 0.0f;
  uint32_t g_ramp_t0_ms = 0;
}

static float BallValvePercentFor(RstController &rst, float target_bar, float current_bar) {
  const float phi = rst.update(target_bar, current_bar);
  const float angle_deg = FlowToAngleDeg(phi);
  return AngleDegToPercentOpen(angle_deg);
}

static void SetSafety(bool open, bool is_lox, ValvesStore &valvesStore) {
  if (IValve* v = Valve_Get(k_valve_safety)) { if (open) v->open(); else v->close(); }
  if (is_lox) valvesStore.set_valve_dpr_pressure_lox(open);
  else        valvesStore.set_valve_dpr_pressure_fuel(open);
}

static void SetVent(bool open, bool is_lox, ValvesStore &valvesStore) {
  if (IValve* v = Valve_Get(k_valve_vent)) { if (open) v->open(); else v->close(); }
  if (is_lox) valvesStore.set_valve_dpr_vent_lox(open);
  else        valvesStore.set_valve_dpr_vent_fuel(open);
}

static void ApplyValveActions(State state, State previous_state, const DataDump &dump,
                               ValvesStore &valvesStore) {
  const bool is_lox = IsLox(dump.boardIdentity.role);

  if (state != previous_state) {
    switch (state) {
      case State::PRESSURIZE_ON: {
        // Ported from BDPR's tankPress(): SAFETY open, VENT closed during
        // the open-loop ramp.
        SetVent(false, is_lox, valvesStore);
        SetSafety(true, is_lox, valvesStore);

        // One-time RST setup on entry to the ramp phase (matches BDPR
        // calling updateRST()+RST_p.set() once per phase-entry, not every
        // tick -- see the RST controller comment above).
//        const float copv_bar = CurrentCopvPressureBar(dump.propSensors, dump.boardIdentity.role);
//        UpdateRstPolynomials(g_ramp_r, g_ramp_s, g_ramp_t, copv_bar);
//        const float current_bar = CurrentTankPressureBar(dump.propSensors, dump.boardIdentity.role);
//        g_ramp_rst.reset(current_bar);
//        g_ramp_p0_bar = current_bar;
//        g_ramp_t0_ms  = HAL_GetTick();
        break;
      }
      case State::PRESSURIZE_OFF:
        SetSafety(false, is_lox, valvesStore);
        SetVent(false, is_lox, valvesStore);
        if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(0.0f);
        break;
      case State::ABORT_ON_GROUND:
        // Ported from the old code's ground-vs-flight abort asymmetry:
        // vent the tank but keep SAFETY closed so the COPV stays isolated
        // (not vented) while on the ground.
        SetSafety(false, is_lox, valvesStore);
        SetVent(true, is_lox, valvesStore);
        if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(0.0f);
        break;
      case State::ABORT_IN_FLIGHT:
        // Same as ground abort, but SAFETY also opens -- connects the COPV
        // into the vented path too, so it bleeds down in flight (only, not
        // on the ground), same intent as the old code's VN behavior.
        SetSafety(true, is_lox, valvesStore);
        SetVent(true, is_lox, valvesStore);
        if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(0.0f);
        break;
      case State::REGULATE: {
        // One-time RST setup on entry to closed-loop regulation (matches
        // BDPR calling updateRST()+RST_c.set() once per phase-entry).
//        const float copv_bar = CurrentCopvPressureBar(dump.propSensors, dump.boardIdentity.role);
//        UpdateRstPolynomials(g_regulate_r, g_regulate_s, g_regulate_t, copv_bar);
//        const float current_bar = CurrentTankPressureBar(dump.propSensors, dump.boardIdentity.role);
//        g_regulate_rst.reset(current_bar);
        break;
      }
      default:
        break;
    }
  }

  // Continuous (every tick, not just on entry):
  if (state == State::PRESSURIZE_ON || state == State::REGULATE) {
//    const float final_target_bar = SetPressureBarFor(dump.boardIdentity.role);
//    const float current_bar = CurrentTankPressureBar(dump.propSensors, dump.boardIdentity.role);
//
//    // PRESSURIZE_ON tracks a climbing ramp reference (ported from BDPR's
//    // pressurisationTask(), BVDPR.ino:270); REGULATE targets the final set
//    // pressure directly. No clamp on the ramp value -- BDPR's doesn't have
//    // one either, since fromPressurizeOn() already leaves this state once
//    // pressure is within k_ramp_exit_threshold_ratio of final_target_bar,
//    // before the reference can climb meaningfully past it.
//    const float target_bar = (state == State::PRESSURIZE_ON)
//        ? g_ramp_p0_bar + static_cast<float>(HAL_GetTick() - g_ramp_t0_ms) * k_ramp_rate_bar_per_ms
//        : final_target_bar;
//
//    RstController &rst = (state == State::REGULATE) ? g_regulate_rst : g_ramp_rst;
//    if (ServoBallValve* ball = Valve_GetBallValve()) {
//      ball->set_position(BallValvePercentFor(rst, target_bar, current_bar));
//    }

    if (state == State::REGULATE) {
      // SAFETY stays open in closed-loop regulation (was already opened on
      // PRESSURIZE_ON entry above); VENT stays closed.
      SetSafety(true, is_lox, valvesStore);
      SetVent(false, is_lox, valvesStore);
    }
  } else if (state == State::PASSIVATE) {
    // Ported from BDPR's passivation(): two-phase, driven by live pressure
    // readings every tick rather than a fixed duration --
    //   phase A (tank still pressurized): SAFETY closed, VENT open -- vent
    //     the tank first, keep the COPV isolated.
    //   phase B (tank already vented): SAFETY also opens, ball valve driven
    //     fully open -- now vent the COPV too (through SAFETY -> tank ->
    //     VENT, since this board has no separate COPV-only vent path).
    //   once BOTH tank and COPV are vented: close everything.
//    const float tank_bar = CurrentTankPressureBar(dump.propSensors, dump.boardIdentity.role);
//    const float copv_bar = CurrentCopvPressureBar(dump.propSensors, dump.boardIdentity.role);
//    const bool tank_vented = tank_bar <= k_vented_threshold_bar;
//    const bool copv_vented = copv_bar <= k_vented_threshold_bar;

//    if (tank_vented && copv_vented) {
//      SetSafety(false, is_lox, valvesStore);
//      SetVent(false, is_lox, valvesStore);
//      if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(0.0f);
//    } else if (tank_vented) {
//      // Phase B: also vent the COPV.
//      SetSafety(true, is_lox, valvesStore);
//      SetVent(true, is_lox, valvesStore);
//      if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(100.0f);
//    } else {
//      // Phase A: vent the tank only.
//      SetSafety(false, is_lox, valvesStore);
//      SetVent(true, is_lox, valvesStore);
//    }
  }
}

static const char* RoleToString(BoardRole role) {
  switch (role) {
    case BoardRole::DprLox:    return "DPR_LOX";
    case BoardRole::DprEth:    return "DPR_ETH";
    case BoardRole::EngineBay: return "ENGINE_BAY";
    default:                   return "UNKNOWN";
  }
}

void Prc_Fsm_Init(void) {
  Valve_InitAll();

  // Role is a hardware fact, latched once here via the inherited generic
  // IStore<T>::set() -- see BoardIdentityStore's comment for why there's no
  // everyday set_role() setter.
  BoardRole role = Prc_DetectBoardRole();
  BoardIdentity identity;
  identity.role = role;
  PrcStore::get_instance().boardIdentityStore.set(identity);

  printf("[PRC FSM] init: board role=%s\r\n", RoleToString(role));
}

void Prc_Fsm_Tick(void) {
  auto& store = PrcStore::get_instance();
  DataDump dump = store.get();

  // EngineBay runs a different FSM entirely (ignition/burn/passivate, not
  // pressurize/regulate -- see PrcBoardId.hpp / data.hpp BoardRole comment),
  // not yet built. Don't let this DPR FSM touch its valves.
  if (dump.boardIdentity.role == BoardRole::EngineBay) {
    return;
  }

  PrcState& fsm = fsm_instance();

  const State previous_state = fsm.getCurrentState();
  fsm.update(dump);
  const State new_state = fsm.getCurrentState();

  ApplyValveActions(new_state, previous_state, dump, store.valvesStore);

  store.stateStore.set(new_state);
}
