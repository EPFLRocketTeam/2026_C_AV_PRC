//    FILE: ValveList.hpp
// PURPOSE: Central registry of every valve GPIO/PWM channel wired on THIS
//          board (Julian_Dev .ioc pinout), exposed through IValve so calling
//          code (sequencer, abort logic, manual test) can use a single
//          ValveId -> IValve* lookup.
//
// Confirmed valve set per DPR bay (LOX or Eth -- whichever this board is,
// see BoardRole/Prc_DetectBoardRole): 2 solenoids + 1 ball valve, one of
// each per bay (no valve in this set is shared between LOX and Eth):
//   - Vent      : tank-vent solenoid, normally open (Ethanol Tank Venting /
//                 LOX Tank Venting).
//   - SafetyDpr : per-bay solenoid, normally closed (ETH Safety DPR / LOX
//                 Safety DPR) -- gates the ball valve's regulation path,
//                 open during PRESSURIZE_ON/REGULATE, closed otherwise.
//   - BallValve : servo ball valve, this bay's proportional DPR element
//                 (Ethanol Tank DPR / LOX Tank DPR).
//
// The vehicle has multiple boards, each commanding a different subset of the
// mission's valves — this board only owns up to 4 solenoids + 1 PWM ball
// valve, which is why the count here doesn't match the full mission valve
// list. LOX_ON/ETH_ON pins exist on this board's .ioc pinout but are not
// valves this board drives, so they are intentionally not in ValveId.
//
// Sol1_ctrl = SafetyDpr, Sol2_ctrl = Vent is the fixed assignment: wire the
// physical valve cables to match this, not the other way around. Sol3/Sol4
// are spare, unused.
//
// TODO (unverified — confirm against the wiring/schematic before flight):
//   - NC vs NO (FailSafeState) below (Vent=NO, SafetyDpr=NC), but GPIO
//     polarity (active_high) is still ASSUMED, not measured against the
//     real drive circuit.
//   - This board's ball valve has no separate enable/power line — it is
//     driven solely by PWM_BV (PB8/TIM4_CH3); BV_CTRL is unused here.

#ifndef DRIVERS_VALVE_VALVE_LIST_H
#define DRIVERS_VALVE_VALVE_LIST_H

#include "Drivers/Valve/Valve.hpp"
#include "Application/Data/data.hpp"

enum class ValveId : uint8_t {
    Vent,      // Sol2_ctrl, PE8 — solenoid, NO (Ethanol Tank Venting / LOX Tank Venting)
    SafetyDpr, // Sol1_ctrl, PE7 — solenoid, NC (ETH Safety DPR / LOX Safety DPR)
    Sol3,      // Sol3_ctrl, PE9  — spare, unused
    Sol4,      // Sol4_ctrl, PE10 — spare, unused
    BallValve, // PWM_BV PB8/TIM4_CH3 — this board's ball valve (Ethanol Tank DPR / LOX Tank DPR)
    Count
};

// Populates every valve below with its real GPIO/PWM config. Must be called
// once, after GPIO/TIM4 peripheral init (MX_GPIO_Init/MX_TIM4_Init), before
// any valve is used.
void Valve_InitAll();

// Create callbacks on the valves to log whenever their value change
void Valve_SetupCallbacks (prc::BoardRole role);

namespace valves_callbacks {

// Engine
void onChange_MO (bool old_open, bool new_open);
void onChange_ME (bool old_open, bool new_open);

// LOX
void onChange_VO (bool old_open, bool new_open);
void onChange_SO (bool old_open, bool new_open);
void onChange_BO (float old_open, float new_open);

// ETH
void onChange_VE (bool old_open, bool new_open);
void onChange_SE (bool old_open, bool new_open);
void onChange_BE (float old_open, float new_open);

} // namespace valves_callbacks

// Returns nullptr if called before Valve_InitAll() or for an out-of-range id.
IValve* Valve_Get(ValveId id);

// Concrete accessor for the one ball valve on this board, for callers that
// need proportional control (set_position()) beyond what the generic
// IValve open()/close() exposes — e.g. bench testing intermediate positions.
// Returns nullptr if called before Valve_InitAll().
ServoBallValve* Valve_GetBallValve();


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

// ---------------------------------------------------------------------------
// Valve mapping: MO (Main Oxidizer) / ME (Main Ethanol) -- same Sol3/Sol4
// bench-test hookup already established in prc_can.cpp's ApplyCmdValves
// ("LOX main"/"Ethanol main"). Not a confirmed real mission mapping either
// (see that comment), just the one convention this codebase already uses.
// ---------------------------------------------------------------------------
static constexpr ValveId k_valve_mo = ValveId::Sol3;
static constexpr ValveId k_valve_me = ValveId::Sol4;

#endif // DRIVERS_VALVE_VALVE_LIST_H
