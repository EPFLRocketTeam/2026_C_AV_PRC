//    FILE: ValveList.hpp
// PURPOSE: Central registry of every valve GPIO/PWM channel wired on THIS
//          board (Julian_Dev .ioc pinout), exposed through IValve so calling
//          code (sequencer, abort logic, manual test) can use a single
//          ValveId -> IValve* lookup.
//
// The vehicle has multiple boards, each commanding a different subset of the
// mission's 10 valves — this board only owns 6 solenoids + 1 PWM ball valve,
// which is why the count here doesn't match the full mission valve list.
// Sol1_ctrl..Sol4_ctrl / LoxOn / EthOn / BallValve are named after this
// board's .ioc pin labels, not PR_VALVE_* part numbers, because which
// physical valve each pin drives hasn't been confirmed yet.
//
// TODO (unverified — confirm against the wiring/schematic before flight):
//   - Map each ValveId below to its PR_VALVE_* part number once known.
//   - NC vs NO (FailSafeState) and GPIO polarity (active_high) are ASSUMED
//     (NormallyClosed / active_high=true for all 6 solenoids), not measured
//     against the real drive circuit.
//   - Whether BV_CTRL is really a servo power-enable line (as modeled here
//     via ServoBallValveConfig::enable_port) vs. something else (direction?
//     a discrete ball-valve GPIO separate from the servo?) is a guess.

#ifndef DRIVERS_VALVE_VALVE_LIST_H
#define DRIVERS_VALVE_VALVE_LIST_H

#include "Drivers/Valve/Valve.hpp"

enum class ValveId : uint8_t {
    Sol1,      // Sol1_ctrl, PE7  — solenoid, TODO: which PR_VALVE_*?
    Sol2,      // Sol2_ctrl, PE8  — solenoid, TODO: which PR_VALVE_*?
    Sol3,      // Sol3_ctrl, PE9  — solenoid, TODO: which PR_VALVE_*?
    Sol4,      // Sol4_ctrl, PE10 — solenoid, TODO: which PR_VALVE_*?
    LoxOn,     // LOX_ON,   PD4   — solenoid, TODO: which PR_VALVE_*?
    EthOn,     // ETH_ON,   PD3   — solenoid, TODO: which PR_VALVE_*?
    BallValve, // BV_CTRL PB5 (enable) + PWM_BV PB8/TIM4_CH3 — this board's
               // one servo ball valve, TODO: which PR_VALVE_* (PN_LOX or
               // PN_ETH — the other one lives on a different board)?
    Count
};

// Populates every valve below with its real GPIO/PWM config. Must be called
// once, after GPIO/TIM4 peripheral init (MX_GPIO_Init/MX_TIM4_Init), before
// any valve is used.
void Valve_InitAll();

// Returns nullptr if called before Valve_InitAll() or for an out-of-range id.
IValve* Valve_Get(ValveId id);

// Concrete accessor for the one ball valve on this board, for callers that
// need proportional control (set_position()) beyond what the generic
// IValve open()/close() exposes — e.g. bench testing intermediate positions.
// Returns nullptr if called before Valve_InitAll().
ServoBallValve* Valve_GetBallValve();

#endif // DRIVERS_VALVE_VALVE_LIST_H
