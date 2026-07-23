#include "../lmt85_manual_test.hpp"
#include "../LMT85.hpp"
#include "main.h"
#include <cstdio>

using namespace Drivers::LMT85;

/**
 * Manual hardware test for the LMT85 temperature sensor.
 *
 * Prerequisites:
 *   - ADC1 must be initialized by CubeMX-generated code (MX_ADC1_Init),
 *     which configures PB1 (LMT85_Pin, see Core/Inc/main.h) on
 *     ADC_CHANNEL_5, 16-bit resolution. NOTE: this project has no ADC2 --
 *     ADC1 is what's actually wired to this pin/channel here (fixed from
 *     this driver's original ADC2 reference, which doesn't exist in this
 *     project).
 *   - printf must be retargeted to UART/USB CDC for output.
 *
 * Usage:
 *   Call manual_test_lmt85() from main.c after all HAL/MX init functions.
 *   The function loops forever, printing readings once per second.
 *
 * Temperature conversion uses the LMT85's datasheet quadratic transfer
 * function (TI SNIS168E, Eq. 1/2) via Config's default v0_mv/t0_c/
 * a_mv_per_c/b_mv_per_c2 — no placeholders left to fill in here.
 */

int manual_test_lmt85() {
    extern ADC_HandleTypeDef hadc1;

    Config config = {
        .hadc        = &hadc1,
        .adc_channel = ADC_CHANNEL_5,
        .adc_max     = 65535, // 16-bit ADC on STM32H7
        .adc_vref_mv = 3300.0f,
    };

    LMT85Driver sensor(config);

    if (!sensor.init()) {
        printf("[LMT85] ERROR: sensor init failed\r\n");
    }

    printf("[LMT85] Manual test started — reading Vtemp (PB1 / ADC1 CH5)\r\n");

    while (1) {
        LMT85Data data;
        if (sensor.read(data)) {
            printf("[LMT85] ADC: %lu | V: %.1f mV | T: %.2f C\r\n",
                   (unsigned long)data.raw_adc,
                   data.voltage_mv,
                   data.temperature);
        } else {
            printf("[LMT85] READ FAILED\r\n");
        }

        HAL_Delay(1000);
    }

    return 0;
}
