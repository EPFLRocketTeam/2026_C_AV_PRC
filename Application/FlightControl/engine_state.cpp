#include "Application/FlightControl/engine_state.h"
#include "Application/app_timebase.h"
#include "Application/app_printf.h"
#include "Application/FlightControl/prc_can.hpp"

#include "Drivers/Valve/ValveList.hpp"
#include "Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include/prc_intranet/const.hpp"

#include "main.h"
#include "stm32h7xx_hal.h"

#include <cstdio>

namespace pi = prc_intranet;

// ---------------------------------------------------------------------------
// !!! TIMING CONSTANTS BELOW ARE PLACEHOLDERS -- NOT REAL VALUES !!!
//
// Set to a uniform 10 s so dry-run bench testing doesn't blow through the
// whole ignition/burn/passivate sequence in a single tick burst (which is
// what happened at 0 ms) -- still arbitrary, not derived from any real
// mission parameter. Prc_Engine_Fsm_Init() also prints a warning every
// boot until these are replaced with real values. DO NOT FLY with any of
// these still at this placeholder value.
// ---------------------------------------------------------------------------

static constexpr uint32_t k_prechill_duration_ms			= 10000; // PRECHILL DURATION
static constexpr uint32_t k_igniter_duration_ms           	= 10000; // IGNITER DURATION
static constexpr uint32_t k_ignition_delay_ms              	= 10000; // IGNITION DELAY
static constexpr uint32_t k_rampup_duration_ms             	= 10000; // RAMPUP DURATION


// "Total impulse + upper/lower bound timer" -- real cutoff should be
// impulse-integral based (needs chamber pressure/thrust data this board
// doesn't read yet, see prc_p_chamber/prc_p_injector). Using a plain
// elapsed-time upper bound as a placeholder/safety backstop only.

/// TOTAL IMPULSE    ?????????????????????

static constexpr uint32_t k_burn_duration_lower_bound_ms	= 10000;  //IS THIS REALLY NEEDED THOUGH
static constexpr uint32_t k_burn_duration_upper_bound_ms	= 10000;
static constexpr uint32_t k_cutoff_delay_ms                 = 10000; // CUTOFF DELAY
// PASSIVATION_DELAY_NO_COM_PRC, comms-loss watchdog, same idea as DPR's
// k_passivation_delay_no_com_ms (prc_state.cpp), no ported number here.
static constexpr uint32_t k_passivation_delay_no_com_ms    	= 10000;
// In-flight-abort-only "timer" before rejoining WaitForPassivation --
// distinct arrow from PASSIVATION_DELAY_NO_COM_PRC on the diagram.
static constexpr uint32_t k_abort_in_flight_timer_ms       	= 10000;
static constexpr uint32_t k_separation_delay_ms            	= 10000; // SEPARATION DELAY
static constexpr uint32_t k_passivation_fuel_duration_ms   	= 10000; // PASSIVATION FUEL DURATION
static constexpr uint32_t k_interlude_duration_ms          	= 10000; // INTERLUDE DURATION
static constexpr uint32_t k_passivation_ox_duration_ms     	= 10000; // PASSIVATION OX DURATION
static constexpr uint32_t k_depressurize_delay_ms          	= 10000; // DEPRESSURIZE DELAY

static void SetMo(bool open) {
  if (IValve* v = Valve_Get(k_valve_mo)) { if (open) v->open(); else v->close(); }
}
static void SetMe(bool open) {
  if (IValve* v = Valve_Get(k_valve_me)) { if (open) v->open(); else v->close(); }
}

// Igniter_Pin (PB2) -- reconfigured to GPIO_MODE_OUTPUT_PP in
// Core/Src/main.c's MX_GPIO_Init() (was GPIO_MODE_ANALOG, i.e. not driven
// at all). Polarity (active_high) is ASSUMED, not measured against the
// real drive circuit -- same caveat as ValveList.hpp's solenoid polarity
// TODO, confirm before relying on this live.
static void SetIgniter(bool on) {
  HAL_GPIO_WritePin(Igniter_GPIO_Port, Igniter_Pin, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// ---------------------------------------------------------------------------
// ColdflowSequence: manual bench test, entirely separate from
// PrcEngineState/the real ignition FSM above -- runs its own tiny timed
// sequence in parallel, driving the same MO/ME/igniter outputs directly.
// Not mutually exclusive with the real FSM; don't run both at once.
//
// Sequence: PRECHILL (MO open, chill only) -> IGNITER (MO closed, igniter
// on) -> BURN_OPEN (igniter off, MO+ME open together, no delay between
// them, unlike the real FSM's staggered BurnStartMo->BurnStartMe) -> hold
// 1 s -> IDLE (MO+ME closed together). Retriggerable: sending "coldflow"
// again restarts from PRECHILL regardless of current state, via
// Prc_Can_TakeColdflowTrigger()'s edge-detected one-shot flag (a level
// check on intranetCmd would retrigger forever, since it's never
// consumed/cleared elsewhere).
//
// Durations are placeholders, same as the real FSM's -- change when real
// values are known.
// ---------------------------------------------------------------------------

static constexpr uint32_t k_coldflow_prechill_duration_ms = 10000;
static constexpr uint32_t k_coldflow_igniter_duration_ms  = 10000;
static constexpr uint32_t k_coldflow_burn_duration_ms     = 1000;

enum class ColdflowState { Idle, Prechill, Igniter, BurnOpen };

namespace {
  ColdflowState g_coldflow_state = ColdflowState::Idle;
  uint32_t g_coldflow_state_entry_ms = 0;
}

static void ColdflowEnter(ColdflowState state) {
  g_coldflow_state = state;
  g_coldflow_state_entry_ms = HAL_GetTick();
  switch (state) {
    case ColdflowState::Prechill:
      app_printf("[COLDFLOW] -> PRECHILL\r\n");
      SetIgniter(false);
      SetMe(false);
      SetMo(true);
      break;
    case ColdflowState::Igniter:
      app_printf("[COLDFLOW] PRECHILL -> IGNITER\r\n");
      SetMo(false);
      SetIgniter(true);
      break;
    case ColdflowState::BurnOpen:
      app_printf("[COLDFLOW] IGNITER -> BURN_OPEN\r\n");
      SetIgniter(false);
      SetMo(true);
      SetMe(true);
      break;
    case ColdflowState::Idle:
      app_printf("[COLDFLOW] BURN_OPEN -> IDLE (closed)\r\n");
      SetMo(false);
      SetMe(false);
      break;
  }
}

static void ColdflowTick() {
  if (Prc_Can_TakeColdflowTrigger()) {
    ColdflowEnter(ColdflowState::Prechill);
    return;
  }

  const uint32_t elapsed_ms = HAL_GetTick() - g_coldflow_state_entry_ms;
  switch (g_coldflow_state) {
    case ColdflowState::Idle:
      break;
    case ColdflowState::Prechill:
      if (elapsed_ms >= k_coldflow_prechill_duration_ms) ColdflowEnter(ColdflowState::Igniter);
      break;
    case ColdflowState::Igniter:
      if (elapsed_ms >= k_coldflow_igniter_duration_ms) ColdflowEnter(ColdflowState::BurnOpen);
      break;
    case ColdflowState::BurnOpen:
      if (elapsed_ms >= k_coldflow_burn_duration_ms) ColdflowEnter(ColdflowState::Idle);
      break;
  }
}

// ---------------------------------------------------------------------------
// Command helpers
// ---------------------------------------------------------------------------

static bool CmdIs(const DataDump &dump, pi::constants::MessageId id) {
  return dump.intranetCmd.id == static_cast<uint16_t>(id);
}

// broadcast_abort's payload carries a magic safety_key -- the RX callback
// (prc_can.cpp's OnBroadcastAbort) already validates it before latching
// intranetCmd, same as every other safety_key/reset message in this
// codebase, so a plain id match here is sufficient.
static bool AbortCmd(const DataDump &dump) {
  return CmdIs(dump, pi::constants::MessageId::broadcast_abort);
}

// ---------------------------------------------------------------------------
// PrcEngineState
// ---------------------------------------------------------------------------

PrcEngineState::PrcEngineState() { currentState = EngineState::Idle; }
PrcEngineState::~PrcEngineState() {}

EngineState PrcEngineState::fromIdle(DataDump const &dump) {
  if (CmdIs(dump, pi::constants::MessageId::prc_clear_to_ignite)) {
    return EngineState::ClearToIgnite;
  }
  return currentState;
}

EngineState PrcEngineState::fromClearToIgnite(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortOnGround;
  if (CmdIs(dump, pi::constants::MessageId::prc_ignite)) {
    return EngineState::IgnitionPrechill;
  }
  return currentState;
}

EngineState PrcEngineState::fromIgnitionPrechill(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortOnGround;
  if (HAL_GetTick() - state_entry_ms_ >= k_prechill_duration_ms) {
    return EngineState::IgnitionIgniter;
  }
  return currentState;
}

EngineState PrcEngineState::fromIgnitionIgniter(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortOnGround;
  if (HAL_GetTick() - state_entry_ms_ >= k_igniter_duration_ms) {
    return EngineState::IgnitionBurnStartMo;
  }
  return currentState;
}

EngineState PrcEngineState::fromIgnitionBurnStartMo(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortOnGround;
  if (HAL_GetTick() - state_entry_ms_ >= k_ignition_delay_ms) {
    return EngineState::IgnitionBurnStartMe;
  }
  return currentState;
}

EngineState PrcEngineState::fromIgnitionBurnStartMe(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortOnGround;
  if (HAL_GetTick() - state_entry_ms_ >= k_rampup_duration_ms) {
    return EngineState::Burn;
  }
  return currentState;
}

EngineState PrcEngineState::fromBurn(DataDump const &dump) {
  // ABORT_OFF branches end here (BurnStartMe -> Burn is the last one) --
  // from Burn onward, abort routes to ABORT_IN_FLIGHT instead, matching
  // the diagram's ABORT_ON/ABORT_OFF split at engine-on vs engine-off.
  if (AbortCmd(dump)) return EngineState::AbortInFlight;
  // "Total impulse + upper/lower bound timer" -- see the constant's
  // comment above, this is a placeholder elapsed-time check only.
  if (HAL_GetTick() - state_entry_ms_ >= k_burn_duration_upper_bound_ms) {
    return EngineState::BurnStopMo;
  }
  return currentState;
}

EngineState PrcEngineState::fromBurnStopMo(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortInFlight;
  if (HAL_GetTick() - state_entry_ms_ >= k_cutoff_delay_ms) {
    return EngineState::BurnStopMe;
  }
  return currentState;
}

EngineState PrcEngineState::fromBurnStopMe(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortInFlight;
  return EngineState::WaitForPassivation;
}

EngineState PrcEngineState::fromWaitForPassivation(DataDump const &dump) {
  if (AbortCmd(dump)) return EngineState::AbortInFlight;
  if (CmdIs(dump, pi::constants::MessageId::prc_passivate)) {
    return EngineState::PassivationSeparationDelay;
  }
  // Comms-loss watchdog: no explicit PASSIVATE for this long -> passivate
  // anyway (ported intent from the DPR FSM's identical pattern).
  if (HAL_GetTick() - state_entry_ms_ >= k_passivation_delay_no_com_ms) {
    return EngineState::PassivationSeparationDelay;
  }
  return currentState;
}

EngineState PrcEngineState::fromPassivationSeparationDelay(DataDump const &dump) {
  (void)dump;
  if (HAL_GetTick() - state_entry_ms_ >= k_separation_delay_ms) {
    return EngineState::PassivationEth;
  }
  return currentState;
}

EngineState PrcEngineState::fromPassivationEth(DataDump const &dump) {
  (void)dump;
  if (HAL_GetTick() - state_entry_ms_ >= k_passivation_fuel_duration_ms) {
    return EngineState::PassivationCloseMe;
  }
  return currentState;
}

EngineState PrcEngineState::fromPassivationCloseMe(DataDump const &dump) {
  (void)dump;
  if (HAL_GetTick() - state_entry_ms_ >= k_interlude_duration_ms) {
    return EngineState::PassivationLox;
  }
  return currentState;
}

EngineState PrcEngineState::fromPassivationLox(DataDump const &dump) {
  (void)dump;
  if (HAL_GetTick() - state_entry_ms_ >= k_passivation_ox_duration_ms) {
    return EngineState::Shutoff;
  }
  return currentState;
}

EngineState PrcEngineState::fromShutoff(DataDump const &dump) {
  (void)dump;
  // Same DEPRESSURIZE DELAY -> OPEN ME+MO tail on every route (nominal
  // end-of-burn and abort-in-flight both converge here already).
  if (HAL_GetTick() - state_entry_ms_ >= k_depressurize_delay_ms) {
    return EngineState::DepressurizeOpen;
  }
  return currentState;
}

EngineState PrcEngineState::fromDepressurizeOpen(DataDump const &dump) {
  // Terminal, otherwise. Per the FC-level FSM diagram, this board isn't
  // the one that commands the DPR boards to depressurize -- FC is. In the
  // current implementation (2026_C_AV_FC's av_state.cpp) that send is a
  // blind 50s timer from DESCENT entry, timed to roughly match this
  // board's own 5-stage/10s-each passivation sequence, not a live read of
  // this state.
  if (CmdIs(dump, pi::constants::MessageId::prc_reset)) return EngineState::Idle;
  return currentState;
}

EngineState PrcEngineState::fromAbortOnGround(DataDump const &dump) {
  (void)dump;
  return EngineState::AbortOnGroundOxydant;
}

EngineState PrcEngineState::fromAbortOnGroundOxydant(DataDump const &dump) {
  if (CmdIs(dump, pi::constants::MessageId::prc_reset)) return EngineState::Idle;
  if (HAL_GetTick() - state_entry_ms_ >= k_cutoff_delay_ms) {
    return EngineState::AbortOnGroundEthanol;
  }
  return currentState;
}

EngineState PrcEngineState::fromAbortOnGroundEthanol(DataDump const &dump) {
  if (CmdIs(dump, pi::constants::MessageId::prc_reset)) return EngineState::Idle;
  return currentState;
}

EngineState PrcEngineState::fromAbortInFlight(DataDump const &dump) {
  (void)dump;
  return EngineState::AbortInFlightOxydant;
}

EngineState PrcEngineState::fromAbortInFlightOxydant(DataDump const &dump) {
  (void)dump;
  if (HAL_GetTick() - state_entry_ms_ >= k_cutoff_delay_ms) {
    return EngineState::AbortInFlightEthanol;
  }
  return currentState;
}

EngineState PrcEngineState::fromAbortInFlightEthanol(DataDump const &dump) {
  (void)dump;
  // No RESET path per the diagram, in-flight abort always proceeds
  // toward passivation on its own, never back to Idle.
  if (HAL_GetTick() - state_entry_ms_ >= k_abort_in_flight_timer_ms) {
    return EngineState::WaitForPassivation;
  }
  return currentState;
}

void PrcEngineState::update(const DataDump &dump) {
  const EngineState previous_state = currentState;

  switch (currentState) {
    case EngineState::Idle:                   currentState = fromIdle(dump); break;
    case EngineState::ClearToIgnite:          currentState = fromClearToIgnite(dump); break;
    case EngineState::IgnitionPrechill:       currentState = fromIgnitionPrechill(dump); break;
    case EngineState::IgnitionIgniter:        currentState = fromIgnitionIgniter(dump); break;
    case EngineState::IgnitionBurnStartMo:    currentState = fromIgnitionBurnStartMo(dump); break;
    case EngineState::IgnitionBurnStartMe:    currentState = fromIgnitionBurnStartMe(dump); break;
    case EngineState::Burn:                   currentState = fromBurn(dump); break;
    case EngineState::BurnStopMo:             currentState = fromBurnStopMo(dump); break;
    case EngineState::BurnStopMe:             currentState = fromBurnStopMe(dump); break;
    case EngineState::WaitForPassivation:     currentState = fromWaitForPassivation(dump); break;
    case EngineState::PassivationSeparationDelay: currentState = fromPassivationSeparationDelay(dump); break;
    case EngineState::PassivationEth:         currentState = fromPassivationEth(dump); break;
    case EngineState::PassivationCloseMe:     currentState = fromPassivationCloseMe(dump); break;
    case EngineState::PassivationLox:         currentState = fromPassivationLox(dump); break;
    case EngineState::Shutoff:                currentState = fromShutoff(dump); break;
    case EngineState::AbortOnGround:          currentState = fromAbortOnGround(dump); break;
    case EngineState::AbortOnGroundOxydant:   currentState = fromAbortOnGroundOxydant(dump); break;
    case EngineState::AbortOnGroundEthanol:   currentState = fromAbortOnGroundEthanol(dump); break;
    case EngineState::AbortInFlight:          currentState = fromAbortInFlight(dump); break;
    case EngineState::AbortInFlightOxydant:   currentState = fromAbortInFlightOxydant(dump); break;
    case EngineState::AbortInFlightEthanol:   currentState = fromAbortInFlightEthanol(dump); break;
    case EngineState::DepressurizeOpen:       currentState = fromDepressurizeOpen(dump); break;
    default:                                  currentState = EngineState::AbortOnGround;
  }

  if (currentState != previous_state) {
    app_printf("[PRC ENGINE FSM] %s -> %s\r\n",
           stateToString(previous_state).c_str(),
           stateToString(currentState).c_str());

    state_entry_ms_ = HAL_GetTick();
  }
}

std::string PrcEngineState::stateToString(EngineState state) {
  switch (state) {
    case EngineState::Idle:                 return "IDLE";
    case EngineState::ClearToIgnite:        return "CLEAR_TO_IGNITE";
    case EngineState::IgnitionPrechill:     return "IGNITION_PRECHILL";
    case EngineState::IgnitionIgniter:      return "IGNITION_IGNITER";
    case EngineState::IgnitionBurnStartMo:  return "IGNITION_BURN_START_MO";
    case EngineState::IgnitionBurnStartMe:  return "IGNITION_BURN_START_ME";
    case EngineState::Burn:                 return "BURN";
    case EngineState::BurnStopMo:           return "BURN_STOP_MO";
    case EngineState::BurnStopMe:           return "BURN_STOP_ME";
    case EngineState::WaitForPassivation:   return "WAIT_FOR_PASSIVATION";
    case EngineState::PassivationSeparationDelay: return "PASSIVATION_SEPARATION_DELAY";
    case EngineState::PassivationEth:       return "PASSIVATION_ETH";
    case EngineState::PassivationCloseMe:   return "PASSIVATION_CLOSE_ME";
    case EngineState::PassivationLox:       return "PASSIVATION_LOX";
    case EngineState::Shutoff:              return "SHUTOFF";
    case EngineState::AbortOnGround:        return "ABORT_ON_GROUND";
    case EngineState::AbortOnGroundOxydant: return "ABORT_ON_GROUND_OXYDANT";
    case EngineState::AbortOnGroundEthanol: return "ABORT_ON_GROUND_ETHANOL";
    case EngineState::AbortInFlight:        return "ABORT_IN_FLIGHT";
    case EngineState::AbortInFlightOxydant: return "ABORT_IN_FLIGHT_OXYDANT";
    case EngineState::AbortInFlightEthanol: return "ABORT_IN_FLIGHT_ETHANOL";
    case EngineState::DepressurizeOpen:     return "DEPRESSURIZE_OPEN";
    default:                                return "ERROR";
  }
}

// ---------------------------------------------------------------------------
// Valve/igniter actions -- entry actions only (state != previous_state),
// same split as the DPR FSM's ApplyValveActions(). No continuous
// (every-tick) actions here, unlike DPR's ball-valve position update --
// this FSM only ever opens/closes two on/off valves plus the igniter.
// ---------------------------------------------------------------------------

static void ApplyEngineValveActions(EngineState state, EngineState previous_state) {
  if (state == previous_state) return;

  switch (state) {
    case EngineState::IgnitionPrechill:
      SetMo(true);
      break;
    case EngineState::IgnitionIgniter:
      SetMo(false);
      SetIgniter(true);
      break;
    case EngineState::IgnitionBurnStartMo:
      SetMo(true);
      SetIgniter(false);
      break;
    case EngineState::IgnitionBurnStartMe:
      SetMe(true);
      break;
    case EngineState::BurnStopMo:
      SetMo(false);
      break;
    case EngineState::BurnStopMe:
      SetMe(false);
      break;
    case EngineState::PassivationEth:
      SetMe(true);
      break;
    case EngineState::PassivationCloseMe:
      SetMe(false);
      break;
    case EngineState::PassivationLox:
      SetMo(true);
      break;
    case EngineState::Shutoff:
      SetMo(false);
      break;
    case EngineState::AbortOnGroundOxydant:
      SetMo(false);
      break;
    case EngineState::AbortOnGroundEthanol:
      SetMe(false);
      break;
    case EngineState::AbortInFlightOxydant:
      SetMo(false);
      break;
    case EngineState::AbortInFlightEthanol:
      SetMe(false);
      break;
    case EngineState::DepressurizeOpen:
      SetMo(true);
      SetMe(true);
      break;
    default:
      break;
  }
}

// ---------------------------------------------------------------------------
// C++ entry points -- called from PrcState's Prc_Fsm_Init()/Prc_Fsm_Tick()
// (prc_state.cpp) when this board's role is EngineBay.
// ---------------------------------------------------------------------------

static PrcEngineState& engine_fsm_instance() {
  static PrcEngineState inst;
  return inst;
}

void Prc_Engine_Fsm_Init() {
  app_printf("[PRC ENGINE FSM] WARNING: ignition/burn/passivation timing "
         "constants are all placeholders (uniform 10 s, not real values) "
         "-- see engine_state.cpp. DO NOT FLY.\r\n");
}

void Prc_Engine_Fsm_Tick() {
  auto& store = PrcStore::get_instance();
  DataDump dump = store.get(app_timebase_now_ms());

  PrcEngineState& fsm = engine_fsm_instance();

  const EngineState previous_state = fsm.getCurrentState();
  fsm.update(dump);
  const EngineState new_state = fsm.getCurrentState();

  ApplyEngineValveActions(new_state, previous_state);

  // Separate manual bench sequence, runs in parallel with the real FSM
  // above -- see ColdflowSequence comment.
  ColdflowTick();
}

EngineState Prc_Engine_Fsm_GetState() {
  return engine_fsm_instance().getCurrentState();
}
