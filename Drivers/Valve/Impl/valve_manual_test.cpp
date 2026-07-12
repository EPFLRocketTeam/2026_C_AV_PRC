#include "../valve_manual_test.hpp"
#include "../ValveList.hpp"

#include "stm32h7xx_hal.h"

#include <cstdint>
#include <cstdio>

static void exercise(ValveId id)
{
    IValve* valve = Valve_Get(id);
    if (valve == nullptr) {
        return;
    }

    printf("[Valve] %s: open\r\n", valve->name());
    valve->open();
    HAL_Delay(500);

    printf("[Valve] %s: close\r\n", valve->name());
    valve->close();
    HAL_Delay(500);
}

// Sweeps the ball valve through intermediate positions, not just the two
// open()/close() endpoints — set_position() is specific to ServoBallValve
// (proportional DPR control), so it isn't reachable through the generic
// IValve interface exercise() uses above.
static void exercise_ball_valve_positions()
{
    ServoBallValve* ball_valve = Valve_GetBallValve();
    if (ball_valve == nullptr) {
        return;
    }

    static const float percentages[] = { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f, 50.0f, 0.0f };

    for (float percent_open : percentages) {
        printf("[Valve] %s: set_position(%.0f%%)\r\n", ball_valve->name(), percent_open);
        ball_valve->set_position(percent_open);
        HAL_Delay(1000); // give the servo time to actually reach position
    }
}

void Valve_ManualTest(void)
{
    printf("[Valve] Manual test started\r\n");

    Valve_InitAll();

    for (uint8_t i = 0; i < static_cast<uint8_t>(ValveId::Count); ++i) {
        exercise(static_cast<ValveId>(i));
    }

    exercise_ball_valve_positions();

    printf("[Valve] Manual test done\r\n");
}
