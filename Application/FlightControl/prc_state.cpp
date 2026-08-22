#include "Application/FlightControl/prc_state.h"
#include "Application/FlightControl/prc_fsm_c_api.h"
#include "Application/FlightControl/engine_state.h"
#include "Application/app_timebase.h"
#include "Application/app_printf.h"
#include "Application/Control/rst_controller.hpp"
#include "Drivers/PrcBoardId/PrcBoardId.hpp"
#include "Drivers/Valve/ValveList.hpp"
#include "Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include/prc_intranet/const.hpp"

#include "main.h"
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

// TEMPORARY bench override delay -- see fromPressurizeOn().
static constexpr uint32_t k_pressurize_on_bypass_delay_ms = 5000u;

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
static constexpr float k_lox_set_pressure_bar  =2.0f;
static constexpr float k_fuel_set_pressure_bar = 2.0f;

static float SetPressureBarFor(BoardRole role) {
  return (role == BoardRole::DprLox) ? k_lox_set_pressure_bar : k_fuel_set_pressure_bar;
}

static bool IsLox(BoardRole role) { return role == BoardRole::DprLox; }

// Role-aware message id selection -- dpr_lox_* and dpr_eth_* are distinct
// messages in the CAN dictionary (see prc_intranet/message_list.hpp), and
// prc_can.cpp's RX callbacks already decode each into intranetCmd.id
// role-gated (OnDprLoxAbort only fires for BoardRole::DprLox, etc). The
// fromXxx() functions below just need to check the id matching this
// board's own role instead of a hardcoded one.
static prc_intranet::constants::MessageId PressurizeIdFor(BoardRole role) {
  return IsLox(role) ? prc_intranet::constants::MessageId::dpr_lox_pressurize
                      : prc_intranet::constants::MessageId::dpr_eth_pressurize;
}
static prc_intranet::constants::MessageId AbortIdFor(BoardRole role) {
  return IsLox(role) ? prc_intranet::constants::MessageId::dpr_lox_abort
                      : prc_intranet::constants::MessageId::dpr_eth_abort;
}
static prc_intranet::constants::MessageId PassivateIdFor(BoardRole role) {
  return IsLox(role) ? prc_intranet::constants::MessageId::dpr_lox_passivate
                      : prc_intranet::constants::MessageId::dpr_eth_passivate;
}
static prc_intranet::constants::MessageId ResetIdFor(BoardRole role) {
  return IsLox(role) ? prc_intranet::constants::MessageId::dpr_lox_reset
                      : prc_intranet::constants::MessageId::dpr_eth_reset;
}

// True on either this board's own role-specific abort message or the
// FC-wide broadcast_abort (OnBroadcastAbort in prc_can.cpp sets this same
// intranetCmd regardless of role) -- previously only the (wrong) hardcoded
// dpr_eth_pressurize id was checked, so broadcast_abort was received and
// decoded but never actually acted on by this FSM.
static bool IsAbortCmd(DataDump const &dump) {
  const uint16_t id = dump.intranetCmd.id;
  return id == (uint16_t)AbortIdFor(dump.boardIdentity.role)
      || id == (uint16_t)prc_intranet::constants::MessageId::broadcast_abort;
}

// Tank/COPV are read from the mean field (filtered), not the raw per-sensor
// ones. propSensorsLox/propSensorsEth are separate structs per board role
// (Application/Data/propulsion/fields.hpp), not one shared PropSensors, so
// this branches on role to pick the right one instead of indexing a common
// field name.
static float CurrentTankPressureBar(const DataDump &dump) {
  return IsLox(dump.boardIdentity.role)
      ? static_cast<float>(dump.propSensorsLox.pressure_OTA_mean)
      : static_cast<float>(dump.propSensorsEth.pressure_ETA_mean);
}

static float CurrentCopvPressureBar(const DataDump &dump) {
  return IsLox(dump.boardIdentity.role)
      ? static_cast<float>(dump.propSensorsLox.pressure_HPO_mean)
      : static_cast<float>(dump.propSensorsEth.pressure_HPE_mean);
}

// Placeholder guess for how long the engine board's own abort shutdown +
// drain sequence takes (engine_state.cpp's AbortInFlight through Shutoff).
// The boards don't share progress over CAN, so this is a timer, not a real
// sync. Same 10 s placeholder convention as engine_state.cpp's constants.
static constexpr uint32_t k_abort_in_flight_dpr_delay_ms = 10000u;

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
  if (dump.intranetCmd.id == (uint16_t)PressurizeIdFor(dump.boardIdentity.role) && dump.intranetCmd.value == 1) {
    return State::PRESSURIZE_ON;
  }
  return currentState;
}

State PrcState::fromPressurizeOn(DataDump const &dump) {
  if (IsAbortCmd(dump)) {
    return State::ABORT_ON_GROUND;
  }

  // TEMPORARY bench override: go to REGULATE after a fixed delay instead of
  // waiting for the real exit condition below. Without real pressurant,
  // measured tank pressure never actually reaches k_ramp_exit_threshold_ratio
  // of the set pressure, so PRESSURIZE_ON would otherwise never exit on its
  // own. Remove this early return to restore the real behavior.
  if (HAL_GetTick() - pressurize_on_entry_ms_ >= k_pressurize_on_bypass_delay_ms) {
    return State::REGULATE;
  }

  // Real behavior: exit the open-loop ramp once measured tank pressure is
  // within k_ramp_exit_threshold_ratio of the final set pressure -- ported
  // from BDPR's tankPress() exit check (BVDPR.ino:517), not a fixed
  // duration.
  const float target_bar  = SetPressureBarFor(dump.boardIdentity.role);
  const float current_bar = CurrentTankPressureBar(dump);
  if (current_bar >= k_ramp_exit_threshold_ratio * target_bar) {
    return State::REGULATE;
  }

  return currentState;
}

State PrcState::fromRegulate(DataDump const &dump) {
  if (IsAbortCmd(dump)) {
    return State::ABORT_ON_GROUND;
  }
  if (dump.intranetCmd.id == (uint16_t)PressurizeIdFor(dump.boardIdentity.role) && dump.intranetCmd.value == 0) {
    return State::PRESSURIZE_OFF;
  }
  return currentState;
}

State PrcState::fromPressurizeOff(DataDump const &dump) {
  // Note: per the FSM diagram, ABORT from this state goes to
  // ABORT_IN_FLIGHT, not ABORT_ON_GROUND (unlike PRESSURIZE_ON/REGULATE) --
  // intentional, matches the old code's PRESSURIZATION_OFF having no ABORT
  // branch at all plus the diagram's asymmetric abort targets.
  if (IsAbortCmd(dump)) {
    return State::ABORT_IN_FLIGHT;
  }

  if (dump.intranetCmd.id == (uint16_t)PassivateIdFor(dump.boardIdentity.role)) {
    return State::PASSIVATE;
  }

  // Comms-loss watchdog (ported from the old code's PRESSURIZATION_OFF
  // case): no explicit PASSIVATE command for this long -> passivate anyway.
  const uint32_t elapsed_ms = HAL_GetTick() - pressurize_off_entry_ms_;
  if (elapsed_ms >= k_passivation_delay_no_com_ms) {
    return State::PASSIVATE;
  }

  return currentState;
}

State PrcState::fromPassivate(DataDump const &dump) {
  // Reachable from both the nominal PRESSURIZE_OFF exit and, after the
  // abort timer, from ABORT_IN_FLIGHT -- same depressurize chain either
  // way, per the DPR FSM diagram. RESET works here (not just from
  // ABORT_ON_GROUND) so the abort-in-flight route still has a way back to
  // MANUAL now that it no longer has its own dedicated terminal state.
  if (dump.intranetCmd.id == (uint16_t)ResetIdFor(dump.boardIdentity.role)) {
    return State::MANUAL;
  }
  // ApplyValveActions() still has two active venting phases inside
  // PASSIVATE, driven every tick by live tank/COPV pressure readings
  // (ported from 2026_C_PR_BDPR's passivation()), not a state transition.
  return currentState;
}

State PrcState::fromAbortOnGround(DataDump const &dump) {
  if (dump.intranetCmd.id == (uint16_t)ResetIdFor(dump.boardIdentity.role)) {
    return State::MANUAL;
  }
  return currentState;
}

State PrcState::fromAbortInFlight(DataDump const &dump) {
  (void)dump;
  // Step 1 (Safety and Ball closed) holds for a fixed delay, then rejoins
  // the same depressurize/passivation chain PRESSURIZE_OFF uses, per the
  // DPR FSM diagram (Abort-in-Flight --Timer--> DEPRESSURIZE), rather than
  // a separate abort-only passivation path.
  if (HAL_GetTick() - abort_in_flight_entry_ms_ >= k_abort_in_flight_dpr_delay_ms) {
    return State::PASSIVATE;
  }
  return currentState;
}

void PrcState::update(const DataDump &dump) {
  const State previous_state = currentState;

  switch (currentState) {
    case State::MANUAL:
      currentState = fromManual(dump);
      break;
    case State::PRESSURIZE_ON:
      currentState = fromPressurizeOn(dump);
      break;
    case State::REGULATE:
      currentState = fromRegulate(dump);
      break;
    case State::PRESSURIZE_OFF:
      currentState = fromPressurizeOff(dump);
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
    app_printf("[PRC FSM] %s -> %s\r\n",
           stateToString(previous_state).c_str(),
           stateToString(currentState).c_str());

    const uint32_t now_ms = HAL_GetTick();
    if (currentState == State::PRESSURIZE_ON)  pressurize_on_entry_ms_  = now_ms;
    if (currentState == State::PRESSURIZE_OFF) pressurize_off_entry_ms_ = now_ms;
    if (currentState == State::PASSIVATE)      passivate_entry_ms_      = now_ms;
    if (currentState == State::ABORT_IN_FLIGHT) abort_in_flight_entry_ms_ = now_ms;
  }
}

std::string PrcState::stateToString(State state) {
  switch (state) {
    case State::MANUAL:                  return "MANUAL";
    case State::PRESSURIZE_ON:           return "PRESSURIZE_ON";
    case State::REGULATE:                return "REGULATE";
    case State::PRESSURIZE_OFF:          return "PRESSURIZE_OFF";
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

static void ValveActions(State state, State previous_state, const DataDump &dump,
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
        // tick, see the RST controller comment above).
        {
          const float copv_bar = CurrentCopvPressureBar(dump);
          UpdateRstPolynomials(g_ramp_r, g_ramp_s, g_ramp_t, copv_bar);
          const float current_bar = CurrentTankPressureBar(dump);
          g_ramp_rst.reset(current_bar);
          g_ramp_p0_bar = current_bar;
          g_ramp_t0_ms  = HAL_GetTick();
        }
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
        // Step 1: Safety and Ball closed. Vent is left alone here, it's
        // already closed coming from any normal pre-abort state.
        SetSafety(false, is_lox, valvesStore);
        if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(0.0f);
        break;
      case State::REGULATE: {
        // One-time RST setup on entry to closed-loop regulation (matches
        // BDPR calling updateRST()+RST_c.set() once per phase-entry).
        const float copv_bar = CurrentCopvPressureBar(dump);
        UpdateRstPolynomials(g_regulate_r, g_regulate_s, g_regulate_t, copv_bar);
        const float current_bar = CurrentTankPressureBar(dump);
        g_regulate_rst.reset(current_bar);
        break;
      }
      default:
        break;
    }
  }

  // Continuous (every tick, not just on entry):
  if (state == State::PRESSURIZE_ON || state == State::REGULATE) {
    const float final_target_bar = SetPressureBarFor(dump.boardIdentity.role);
    const float current_bar = CurrentTankPressureBar(dump);

    // PRESSURIZE_ON tracks a climbing ramp reference (ported from BDPR's
    // pressurisationTask(), BVDPR.ino:270); REGULATE targets the final set
    // pressure directly. No clamp on the ramp value, BDPR's doesn't have
    // one either, since fromPressurizeOn() already leaves this state once
    // pressure is within k_ramp_exit_threshold_ratio of final_target_bar,
    // before the reference can climb meaningfully past it.
    const float target_bar = (state == State::PRESSURIZE_ON)
        ? g_ramp_p0_bar + static_cast<float>(HAL_GetTick() - g_ramp_t0_ms) * k_ramp_rate_bar_per_ms
        : final_target_bar;

    RstController &rst = (state == State::REGULATE) ? g_regulate_rst : g_ramp_rst;
    if (ServoBallValve* ball = Valve_GetBallValve()) {
      // dither=false: this runs every tick, dithering here would fight the
      // control loop instead of settling it (see Valve.hpp's comment).
      ball->set_position(BallValvePercentFor(rst, target_bar, current_bar), false);
    }

    if (state == State::REGULATE) {
      // SAFETY stays open in closed-loop regulation (was already opened on
      // PRESSURIZE_ON entry above); VENT stays closed.
      SetSafety(true, is_lox, valvesStore);
      SetVent(false, is_lox, valvesStore);
    }
  } else if (state == State::PASSIVATE) {
    // Simplified from BDPR's two-phase passivation() (tank-then-COPV,
    // live-pressure-driven): just open everything at once instead.
    SetSafety(true, is_lox, valvesStore);
    SetVent(true, is_lox, valvesStore);
    if (ServoBallValve* ball = Valve_GetBallValve()) ball->set_position(100.0f);
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

// Blocking, boot-time only, same reasoning as the other one-shot init-time
// hardware tests in this codebase. Audible board-role confirmation: 3 beeps
// for Engine, 2 for DPR-LOX, 1 for DPR-ETH.
static void BeepBuzzer(int count) {
  for (int i = 0; i < count; i++) {
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
    HAL_Delay(150);
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
    HAL_Delay(150);
  }
}

static int BeepCountFor(BoardRole role) {
  switch (role) {
    case BoardRole::EngineBay: return 3;
    case BoardRole::DprLox:    return 2;
    case BoardRole::DprEth:    return 1;
    default:                   return 0;
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

  app_printf("[PRC FSM] init: board role=%s\r\n", RoleToString(role));

  BeepBuzzer(BeepCountFor(role));

  if (role == BoardRole::EngineBay) {
    Prc_Engine_Fsm_Init();
  }
}

void Prc_Fsm_Tick(void) {
  auto& store = PrcStore::get_instance();
  DataDump dump = store.get(app_timebase_now_ms());

  // EngineBay runs a different FSM entirely (ignition/burn/passivate, not
  // pressurize/regulate -- see PrcBoardId.hpp / data.hpp BoardRole comment
  // and engine_state.cpp/"Propulsion Computer FSM" diagram).
  if (dump.boardIdentity.role == BoardRole::EngineBay) {
    Prc_Engine_Fsm_Tick();
    return;
  }

  PrcState& fsm = fsm_instance();

  const State previous_state = fsm.getCurrentState();
  fsm.update(dump);
  const State new_state = fsm.getCurrentState();

  ValveActions(new_state, previous_state, dump, store.valvesStore);

  store.stateStore.set(new_state);
}

// Manual valve override for ground-station bench commands (VENT LOX/FUEL,
// PRESSURE LOX/FUEL, VENT_COPV, see prc_can.cpp's ApplyCmdValves). Per spec,
// GSC keeps manual valve control through FILLING, ARMED, and PRESSURIZATION,
// so this covers MANUAL plus the ramp/regulate states, not just MANUAL.
// Note that REGULATE keeps reasserting Safety/Vent every tick on its own
// (ApplyValveActions), so a manual Safety/Vent command there won't hold;
// the ball valve isn't affected by that and works normally.
static bool ManualOverrideAllowed(State state) {
  return state == State::MANUAL
      || state == State::PRESSURIZE_ON
      || state == State::REGULATE;
}

bool Prc_Fsm_ManualSetSafety(bool open) {
  if (!ManualOverrideAllowed(fsm_instance().getCurrentState())) return false;
  auto& store = PrcStore::get_instance();
  SetSafety(open, IsLox(store.boardIdentityStore.get_role()), store.valvesStore);
  return true;
}

bool Prc_Fsm_ManualSetVent(bool open) {
  if (!ManualOverrideAllowed(fsm_instance().getCurrentState())) return false;
  auto& store = PrcStore::get_instance();
  SetVent(open, IsLox(store.boardIdentityStore.get_role()), store.valvesStore);
  return true;
}

// This board has no standalone COPV vent line, gas can only exit via
// Vent, then Safety, then the ball valve (same sequence as PASSIVATE's
// phase B in ApplyValveActions, just triggered manually here).
bool Prc_Fsm_ManualVentCopv(bool open) {
  if (!ManualOverrideAllowed(fsm_instance().getCurrentState())) return false;
  auto& store = PrcStore::get_instance();
  const bool is_lox = IsLox(store.boardIdentityStore.get_role());
  SetSafety(open, is_lox, store.valvesStore);
  SetVent(open, is_lox, store.valvesStore);
  if (ServoBallValve* ball = Valve_GetBallValve()) {
    ball->set_position(open ? 25.0f : 0.0f);
  }
  return true;
}

bool Prc_Fsm_ManualSetBallValve(float percent_open) {
  if (!ManualOverrideAllowed(fsm_instance().getCurrentState())) return false;
  ServoBallValve* ball = Valve_GetBallValve();
  if (!ball) return false;
  return ball->set_position(percent_open) == ValveStatus::Ok;
}
