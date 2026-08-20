
#ifndef APP_PRC_FSM_H
#define APP_PRC_FSM_H

namespace prc {
// Dynamic Pressure Regulator FSM (see 2026_C_SE_FLIGHT-PARAMETERS,
// "Dynamic Pressure Regulator FSM" diagram). Mirrors flight_computer::State
// (Application/Data/fsm.hpp in 2026_C_AV_FC) in structure, but this is this
// board's own local FSM, not the mission-wide FC state.
// The old code's INITIALIZE_PRESSURIZE_ON/INITIALIZE_REGULATE/
// INITIALIZE_PASSIVATE one-time-setup states (ported from the previous
// vehicle's DPRComputer.cpp, each falling straight through to its
// corresponding steady state) were all removed since none of them did any
// real setup work -- they only added pass-through ticks.
enum State {
  MANUAL,

  PRESSURIZE_ON,
  REGULATE,
  PRESSURIZE_OFF,
  PASSIVATE,

  /* ============== Error States ============== */
  ABORT_ON_GROUND,
  ABORT_IN_FLIGHT, // step 1: Safety + Ball closed, then rejoins PASSIVATE via a timer
};
};

#endif /* APP_PRC_FSM_H */
