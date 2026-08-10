// This is the header file for the PrcEngineState class -- the Engine Bay
// (PRC-P) FSM, mirroring PrcState's structure (prc_state.h) but running
// the ignition/burn/passivate sequence instead of pressurize/regulate.
// Ported from the "Propulsion Computer FSM" diagram.
//
// Named PrcEngineState (not EngineState) to avoid colliding with the
// prc::EngineState enum (engine_fsm.hpp) once `using namespace prc;` is in
// scope -- same reasoning as PrcState vs prc::State.

#ifndef PRC_ENGINE_STATE_H
#define PRC_ENGINE_STATE_H

#include <cstdint>
#include <string>

#include "Application/Data/data.hpp"
#include "Application/Data/engine_fsm.hpp"

using namespace prc;

class PrcEngineState {
public:
  explicit PrcEngineState();
  ~PrcEngineState();

  EngineState getCurrentState() const { return currentState; }

  void update(const DataDump &dump);

  static std::string stateToString(EngineState state);

private:
  EngineState fromIdle(DataDump const &dump);
  EngineState fromClearToIgnite(DataDump const &dump);
  EngineState fromIgnitionPrechill(DataDump const &dump);
  EngineState fromIgnitionIgniter(DataDump const &dump);
  EngineState fromIgnitionBurnStartMo(DataDump const &dump);
  EngineState fromIgnitionBurnStartMe(DataDump const &dump);
  EngineState fromBurn(DataDump const &dump);
  EngineState fromBurnStopMo(DataDump const &dump);
  EngineState fromBurnStopMe(DataDump const &dump);
  EngineState fromWaitForPassivation(DataDump const &dump);
  EngineState fromPassivationEth(DataDump const &dump);
  EngineState fromPassivationCloseMe(DataDump const &dump);
  EngineState fromPassivationLox(DataDump const &dump);
  EngineState fromShutoff(DataDump const &dump);
  EngineState fromAbortOnGround(DataDump const &dump);
  EngineState fromAbortOnGroundOxydant(DataDump const &dump);
  EngineState fromAbortOnGroundEthanol(DataDump const &dump);
  EngineState fromAbortInFlight(DataDump const &dump);
  EngineState fromAbortInFlightOxydant(DataDump const &dump);
  EngineState fromAbortInFlightEthanol(DataDump const &dump);

  EngineState currentState;

  // HAL_GetTick() timestamp of entry into currentState -- refreshed in
  // update() any time currentState actually changes. Almost every state
  // here has its own duration-based auto-transition (unlike PrcState,
  // which only needed a handful of named timestamps), so one generic
  // "time we entered the current state" value is simpler than a member
  // per state.
  uint32_t state_entry_ms_ = 0;
};

// C++ entry points, called from PrcState's Prc_Fsm_Init()/Prc_Fsm_Tick()
// (prc_state.cpp) when this board's latched role is EngineBay -- same
// split as prc_fsm_c_api.h's C-callable wrapper around the DPR FSM, one
// level up.
void Prc_Engine_Fsm_Init();
void Prc_Engine_Fsm_Tick();

// Exposes the running engine FSM's current state to prc_can.cpp for
// telemetry (Prc_Can_SendTelemetry's EngineBay branch), without making
// the singleton instance itself public.
EngineState Prc_Engine_Fsm_GetState();

#endif // PRC_ENGINE_STATE_H
