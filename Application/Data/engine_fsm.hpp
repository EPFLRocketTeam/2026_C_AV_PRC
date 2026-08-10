
#ifndef APP_PRC_ENGINE_FSM_H
#define APP_PRC_ENGINE_FSM_H

namespace prc {

// Engine bay (PRC-P) FSM -- see "Propulsion Computer FSM" diagram. Separate
// from prc::State (the DPR pressurize/regulate/passivate FSM, fsm.hpp):
// this board runs ignition/burn/passivate instead. Kept as its own
// `enum class` (not the DPR State's unscoped `enum`) so enumerator names
// can't collide with it -- both live in namespace prc.
//
// The diagram's two composite boxes (IGNITION SQ, PASSIVATION SQ) are
// flattened into their individual internal states here, same way the DPR
// FSM flattens INITIALIZE_* "setup states" rather than modeling real
// hierarchical states -- see engine_state.cpp's EngineState::update().
enum class EngineState {
  Idle,
  ClearToIgnite,

  // IGNITION SQ
  IgnitionPrechill,       // open MO
  IgnitionIgniter,        // close MO + igniter ON
  IgnitionBurnStartMo,    // open MO + igniter OFF
  IgnitionBurnStartMe,    // open ME

  Burn,
  BurnStopMo,             // close MO
  BurnStopMe,             // close ME
  WaitForPassivation,

  // PASSIVATION SQ
  PassivationEth,         // open ME
  PassivationCloseMe,     // close ME
  PassivationLox,         // open MO
  Shutoff,                // close MO -- terminal

  AbortOnGround,
  AbortOnGroundOxydant,   // close MO
  AbortOnGroundEthanol,   // close ME -- then waits for RESET -> Idle

  AbortInFlight,
  AbortInFlightOxydant,   // close MO
  AbortInFlightEthanol,   // close ME -- then times out into WaitForPassivation
};

} // namespace prc

#endif // APP_PRC_ENGINE_FSM_H
