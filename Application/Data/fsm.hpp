
#ifndef APP_PRC_FSM_H
#define APP_PRC_FSM_H

namespace prc {
// Dynamic Pressure Regulator FSM (see 2026_C_SE_FLIGHT-PARAMETERS,
// "Dynamic Pressure Regulator FSM" diagram). Mirrors flight_computer::State
// (Application/Data/fsm.hpp in 2026_C_AV_FC) in structure, but this is this
// board's own local FSM, not the mission-wide FC state.
// INITIALIZE_* are one-time-setup states (ported from the previous
// vehicle's DPRComputer.cpp): each runs its setup once (zero the PID
// integrator/derivative, capture start time / initial COPV pressure) then
// immediately falls through to the corresponding steady state, same as the
// old code's INITIALIZE_PRESSURIZATION/INITIALIZE_REGULATION/
// INITIALIZE_PASSIVATION -> PRESSURIZATION/REGULATION/PASSIVATION pattern.
enum State {
  MANUAL,

  INITIALIZE_PRESSURIZE_ON,
  PRESSURIZE_ON,
  INITIALIZE_REGULATE,
  REGULATE,
  PRESSURIZE_OFF,
  INITIALIZE_PASSIVATE,
  PASSIVATE,

  /* ============== Error States ============== */
  ABORT_ON_GROUND,
  ABORT_IN_FLIGHT,
};
};

#endif /* APP_PRC_FSM_H */
