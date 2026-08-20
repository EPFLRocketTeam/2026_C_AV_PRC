// This is the header file for the PrcState class — the Dynamic Pressure
// Regulator FSM for this board, mirroring the structure of
// 2026_C_AV_FC's AvState (Application/FlightControl/av_state.h): one
// fromXxx() transition function per state, called from update() via a
// snapshot (DataDump) of this tick's sensors/commands/timers.
//
// The state/transition logic (ramp-up, comms-loss passivation watchdog,
// abort valve sequences) is ported from the previous vehicle's
// DPRComputer.cpp/.h (Arduino), translated to STM32 HAL + this project's
// PrcState/PrcStore pattern. The old vehicle PWM'd an on/off solenoid (PN)
// with a PID loop to fake proportional flow control; this board has a real
// proportional device (ServoBallValve, see Drivers/Valve/Valve.hpp), so
// regulation here is direct position control, not a ported PID + duty
// cycle. PN's role is now just a simple isolation valve (see
// ApplyValveActions in prc_state.cpp for the full PN/VX/VN -> Valves
// field mapping).

#ifndef PRC_STATE_H
#define PRC_STATE_H

#include <cstdint>
#include <string>

#include "Application/Data/data.hpp"
#include "Application/Data/fsm.hpp"

using namespace prc;

class PrcState {
public:
  explicit PrcState();
  ~PrcState();

  /**
   * @brief Return the FSM's current state.
   */
  State getCurrentState();

  /**
   * @brief Evaluate the current sensor/event snapshot and advance the FSM.
   * @param dump Snapshot of this board's data at the current tick.
   */
  void update(const DataDump &dump);

  /**
   * @brief Convert a @c State enum value to a human-readable string.
   */
  std::string stateToString(State state);

  /**
   * @brief HAL_GetTick() timestamp of the most recent entry into PASSIVATE
   * (0 if never entered). Not currently consumed by anything -- PASSIVATE's
   * venting phases are now driven by live pressure readings each tick (see
   * ApplyValveActions in prc_state.cpp), not elapsed time. Kept for
   * telemetry/logging ("how long have we been passivating").
   */
  uint32_t getPassivateEntryMs() const { return passivate_entry_ms_; }

private:
  State fromManual(DataDump const &dump);
  State fromPressurizeOn(DataDump const &dump);
  State fromRegulate(DataDump const &dump);
  State fromPressurizeOff(DataDump const &dump);
  State fromPassivate(DataDump const &dump);
  State fromAbortOnGround(DataDump const &dump);
  State fromAbortInFlight(DataDump const &dump);

  // Open-loop ramp tracking (dpr_memory_controller_t's rampedPressure
  // equivalent) is implemented, but as file-scope state in prc_state.cpp
  // (g_ramp_p0_bar/g_ramp_t0_ms, next to the RST controller instances it
  // feeds) rather than as members here -- ApplyValveActions() is a free
  // function with no access to PrcState's private state, and the ramp is
  // consumed there, not in this class's own transition logic (only the
  // ramp *exit check* lives in fromPressurizeOn()).

  State currentState;

  // HAL_GetTick() timestamps for timed transitions (ramp-up, passivation
  // hold, comms-loss watchdog). Set on entry to the corresponding state in
  // update().
  uint32_t pressurize_on_entry_ms_  = 0;
  uint32_t passivate_entry_ms_      = 0;
  uint32_t pressurize_off_entry_ms_ = 0; // drives the comms-loss auto-passivate watchdog
  uint32_t abort_in_flight_entry_ms_ = 0; // drives the ABORT_IN_FLIGHT -> PASSIVATE timer
};

// See prc_state.cpp for the full comment -- all three are no-ops (return
// false) unless this board's FSM is currently in State::MANUAL.
bool Prc_Fsm_ManualSetSafety(bool open);
bool Prc_Fsm_ManualSetVent(bool open);
bool Prc_Fsm_ManualVentCopv(bool open);
bool Prc_Fsm_ManualSetBallValve(float percent_open);

#endif /* PRC_STATE_H */
