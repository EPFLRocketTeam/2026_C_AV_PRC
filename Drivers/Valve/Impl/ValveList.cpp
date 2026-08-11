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
    // Fail-safe state matches PR's confirmed part list (Vent=NO,
    // SafetyDpr=NC); active_high is still ASSUMED (see ValveList.hpp TODO)
    // — verify against the actual drive circuit before relying on this for
    // anything live.
    static SolenoidValve safety_dpr({
        .port      = Sol1_ctrl_GPIO_Port,
        .pin       = Sol1_ctrl_Pin,
        .fail_safe = FailSafeState::NormallyClosed,
        .name      = "SafetyDpr",
    });
    static SolenoidValve vent({
        .port      = Sol2_ctrl_GPIO_Port,
        .pin       = Sol2_ctrl_Pin,
        .fail_safe = FailSafeState::NormallyOpen,
        .name      = "Vent",
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
