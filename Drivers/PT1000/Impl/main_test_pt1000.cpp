#include "main_test_pt1000.h"
#include "../PT1000.hpp"
#include "main.h"
#include <cstdio>

using namespace Drivers::PT1000;

/**
 * Manual hardware test for the 4 PT1000 temperature sensors.
 *
 * Prerequisites:
 *   - ADC peripherals must be initialized by CubeMX-generated code
 *     (MX_ADC1_Init / MX_ADC3_Init or equivalent).
 *   - printf must be retargeted to UART or SWO for output.
 *   - HAL_Delay must be available.
 *
 * Usage:
 *   Call manual_test_pt1000() from main.c after all HAL/MX init functions.
 *   The function loops forever, printing readings once per second.
 *
 * TODO: Replace the placeholder ADC handles and channels below with
 *       the actual values from the Firehorn II schematics once confirmed.
 */

int manual_test_pt1000() {
    // ===================================================================
    // CONFIGURATION
    // ===================================================================
    //
    // All 4 sensors run on ADC3:
    //   PT1000-1 -> PC0    -> ADC3 channel 10
    //   PT1000-2 -> PC1    -> ADC3 channel 11
    //   PT1000-3 -> PC2_C  -> ADC3 channel 0  (dedicated analog-switch pin, ADC3-only)
    //   PT1000-4 -> PC3_C  -> ADC3 channel 1  (dedicated analog-switch pin, ADC3-only)
    //
    // The reference resistor (r_ref) value must match the resistor in
    // the voltage divider on the PCB (e.g. 4.7 kΩ = 4700.0f).
    // ===================================================================

    extern ADC_HandleTypeDef hadc3;

    Config configs[NUM_SENSORS] = {
        // PT1000-1 (PC0, ADC3 channel 10)
        { .hadc = &hadc3,
          .adc_channel = 10,
          .r_ref = 1100.0f,
          .adc_max = 65535 },  // 16-bit ADC on STM32H7
        // PT1000-2 (PC1, ADC3 channel 11)
        { .hadc = &hadc3,
          .adc_channel = 11,
          .r_ref = 1100.0f,
          .adc_max = 65535 },
        // PT1000-3 (PC2_C, ADC3 channel 0)
        { .hadc = &hadc3,
          .adc_channel = 0,
          .r_ref = 1100.0f,
          .adc_max = 65535 },
        // PT1000-4 (PC3_C, ADC3 channel 1)
        { .hadc = &hadc3,
          .adc_channel = 1,
          .r_ref = 1100.0f,
          .adc_max = 65535 },
    };

    PT1000Driver sensors[NUM_SENSORS] = {
        PT1000Driver(configs[0], 1),
        PT1000Driver(configs[1], 2),
        PT1000Driver(configs[2], 3),
        PT1000Driver(configs[3], 4),
    };

    // Initialize all sensors
    for (uint8_t i = 0; i < NUM_SENSORS; i++) {
        if (!sensors[i].init()) {
            printf("[PT1000] ERROR: sensor %u init failed\r\n", i + 1);
        }
    }

    printf("[PT1000] Manual test started — reading 4 channels\r\n");

    // Infinite read loop
    while (1) {
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            PT1000Data data;
            if (sensors[i].read(data)) {
                printf("[PT1000-%u] ADC: %lu | R: %.1f Ohm | T: %.2f C\r\n",
                       data.sensor_id,
                       (unsigned long)data.raw_adc,
                       data.resistance,
                       data.temperature);
            } else {
                printf("[PT1000-%u] READ FAILED\r\n", i + 1);
            }
        }
        printf("---\r\n");

        HAL_Delay(1000);
    }

    return 0;
}
