//    FILE: ValveList.hpp
// PURPOSE: Central registry of every valve GPIO/PWM channel wired on THIS
//          board (Julian_Dev .ioc pinout), exposed through IValve so calling
//          code (sequencer, abort logic, manual test) can use a single
//          ValveId -> IValve* lookup.
//
// The vehicle has multiple boards, each commanding a different subset of the
// mission's valves — this board only owns 4 solenoids + 1 PWM ball valve,
// which is why the count here doesn't match the full mission valve list.
// LOX_ON/ETH_ON pins exist on this board's .ioc pinout but are not valves
// this board drives, so they are intentionally not in ValveId.
// Sol1_ctrl..Sol4_ctrl / BallValve are named after this board's .ioc pin
// labels, not PR_VALVE_* part numbers, because which physical valve each pin
// drives hasn't been confirmed yet.
//
// TODO (unverified — confirm against the wiring/schematic before flight):
//   - Map each ValveId below to its PR_VALVE_* part number once known.
//   - NC vs NO (FailSafeState) and GPIO polarity (active_high) are ASSUMED
//     (NormallyClosed / active_high=true for all 4 solenoids), not measured
//     against the real drive circuit.
//   - This board's ball valve has no separate enable/power line — it is
//     driven solely by PWM_BV (PB8/TIM4_CH3); BV_CTRL is unused here.

#ifndef DRIVERS_VALVE_VALVE_LIST_H
#define DRIVERS_VALVE_VALVE_LIST_H

#include "Drivers/Valve/Valve.hpp"

enum class ValveId : uint8_t {
    Sol1,      // Sol1_ctrl, PE7  — solenoid, TODO: which PR_VALVE_*?
    Sol2,      // Sol2_ctrl, PE8  — solenoid, TODO: which PR_VALVE_*?
    Sol3,      // Sol3_ctrl, PE9  — solenoid, TODO: which PR_VALVE_*?
    Sol4,      // Sol4_ctrl, PE10 — solenoid, TODO: which PR_VALVE_*?
    BallValve, // PWM_BV PB8/TIM4_CH3 — this board's one servo ball valve
               // (no separate enable line), TODO: which PR_VALVE_* (PN_LOX
               // or PN_ETH — the other one lives on a different board)?
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
