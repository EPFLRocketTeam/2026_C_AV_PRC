#include "Drivers/Valve/ValveList.hpp"

#include "main.h"

#include <cstddef>

// TIM4 is configured for PWM (channel 3, PWM_BV) in Core/Src/main.c
// (MX_TIM4_Init); its handle is a plain global there, not exported via
// main.h, so extern it directly.
extern TIM_HandleTypeDef htim4;

// Storage for the concrete valve objects. Solenoids and servo are kept in
// separate arrays purely to avoid needing a variant/union; Valve_Get()
// returns them all through the common IValve* interface.
static SolenoidValve*  s_solenoids[4] = { nullptr };
static ServoBallValve* s_servo        = nullptr;
static bool            s_initialized  = false;

void Valve_InitAll()
{
    // Fail-safe state matches the part labels (Vent=NO, SafetyDpr=NC).
    // Vent's active_high was confirmed false by testing (open/close were
    // inverted at active_high=true). SafetyDpr/Sol3/Sol4 still assume
    // active_high=true, unverified.
    static SolenoidValve safety_dpr({
        .port      = Sol3_ctrl_GPIO_Port,
        .pin       = Sol3_ctrl_Pin,
        .fail_safe = FailSafeState::NormallyClosed,
        .name      = "SafetyDpr",
    });
    static SolenoidValve vent({
        .port        = Sol2_ctrl_GPIO_Port,
        .pin         = Sol2_ctrl_Pin,
        .fail_safe   = FailSafeState::NormallyOpen,
        .active_high = true,
        .name        = "Vent",
    });
    static SolenoidValve sol3({
        .port      = Sol3_ctrl_GPIO_Port,
        .pin       = Sol3_ctrl_Pin,
        .fail_safe = FailSafeState::NormallyClosed,
        .name      = "Sol3_ctrl",
    });
    static SolenoidValve sol4({
        .port      = Sol4_ctrl_GPIO_Port,
        .pin       = Sol4_ctrl_Pin,
        .fail_safe = FailSafeState::NormallyClosed,
        .name      = "Sol4_ctrl",
    });
    s_solenoids[0] = &vent;
    s_solenoids[1] = &safety_dpr;
    s_solenoids[2] = &sol3;
    s_solenoids[3] = &sol4;

    // No separate enable/power line on this board — the servo is driven
    // solely by the PWM_BV signal (PB8/TIM4_CH3), so enable_port/enable_pin
    // are left at their nullptr/0 defaults.
    static ServoBallValve ball_valve({
        .htim        = &htim4,
        .channel     = TIM_CHANNEL_3,
        .name        = "PWM_BV",
    });
    s_servo = &ball_valve;

    // TIM4 leaves the compare register at 0 (an invalid pulse width) until
    // something actually calls set_position(), so drive it to a known
    // closed position as soon as the timer starts, instead of leaving it
    // wherever the floating pre-init signal happened to leave it.
    ball_valve.set_position(0.0f);

    s_initialized = true;
}

IValve* Valve_Get(ValveId id)
{
    if (!s_initialized) {
        return nullptr;
    }

    switch (id) {
        case ValveId::Vent:      return s_solenoids[0];
        case ValveId::SafetyDpr: return s_solenoids[1];
        case ValveId::Sol3:      return s_solenoids[2];
        case ValveId::Sol4:      return s_solenoids[3];
        case ValveId::BallValve: return s_servo;
        default:                 return nullptr;
    }
}

ServoBallValve* Valve_GetBallValve()
{
    return s_initialized ? s_servo : nullptr;
}

void Valve_SetupCallbacks (prc::BoardRole role) {
    switch (role) {
        case prc::BoardRole::EngineBay:
            ((SolenoidValve*) Valve_Get(k_valve_mo))->setCallback(&valves_callbacks::onChange_MO);
            ((SolenoidValve*) Valve_Get(k_valve_me))->setCallback(&valves_callbacks::onChange_ME);
            break ;
        case prc::BoardRole::DprEth:
            ((SolenoidValve*) Valve_Get(k_valve_vent))->setCallback(&valves_callbacks::onChange_VE);
            ((SolenoidValve*) Valve_Get(k_valve_safety))->setCallback(&valves_callbacks::onChange_SE);
            Valve_GetBallValve()->setCallback(&valves_callbacks::onChange_BE);
            break ;
        case prc::BoardRole::DprLox:
            ((SolenoidValve*) Valve_Get(k_valve_vent))->setCallback(&valves_callbacks::onChange_VO);
            ((SolenoidValve*) Valve_Get(k_valve_safety))->setCallback(&valves_callbacks::onChange_SO);
            Valve_GetBallValve()->setCallback(&valves_callbacks::onChange_BO);
            break ;
        case prc::BoardRole::Unknown:
        default:
            break ;
    }
}
