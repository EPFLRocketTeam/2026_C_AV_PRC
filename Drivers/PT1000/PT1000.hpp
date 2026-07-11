#pragma once

#include "stm32h7xx_hal.h"
#include <cstdint>

namespace Drivers {
namespace PT1000 {

    // ======================================================================
    // CONFIGURATION
    // ======================================================================

    struct Config {
        ADC_HandleTypeDef* hadc;       // Pointer to the ADC handle
        uint32_t           adc_channel; // ADC channel number
        float              r_ref;      // Reference resistor in the voltage divider (Ohms)
        uint32_t           adc_max;    // Max ADC value (e.g. 4095 for 12-bit, 65535 for 16-bit)
    };

    // ======================================================================
    // DATA STRUCTURES
    // ======================================================================

    struct PT1000Data {
        float    resistance;       // Measured resistance in Ohms
        float    temperature;      // Temperature in degrees Celsius
        uint32_t raw_adc;          // Raw ADC reading
        uint8_t  sensor_id;        // Sensor index (1-4)
        bool     valid;            // true if the reading is valid
    };

    // ======================================================================
    // CONSTANTS
    // ======================================================================

    /// Resistance of the PT1000 at 0°C (Ohms)
    static constexpr float R0    = 1000.0f;
    /// IEC 60751 standard temperature coefficient (°C⁻¹)
    static constexpr float ALPHA = 0.00385f;
    /// Number of PT1000 sensors on the board
    static constexpr uint8_t NUM_SENSORS = 4;

    // ======================================================================
    // DRIVER CLASS
    // ======================================================================

    class PT1000Driver {
    public:
        /**
         * @brief Construct a PT1000 driver for a single sensor.
         * @param config  ADC handle, channel, reference resistor, resolution.
         * @param sensor_id  Sensor index (1-4).
         */
        PT1000Driver(Config config, uint8_t sensor_id);

        /**
         * @brief Initialize the ADC peripheral for this sensor.
         * @return true on success.
         */
        bool init();

        /**
         * @brief Perform a blocking ADC read and compute temperature.
         * @param out  Output structure filled with resistance, temperature, etc.
         * @return true if the reading is valid.
         */
        bool read(PT1000Data& out);

        /**
         * @brief Convert a raw ADC value to PT1000 resistance.
         *
         * Assumes a voltage divider with R_ref on the high side and
         * PT1000 on the low side (pull-down to GND):
         *   V_adc = V_ref * R_pt1000 / (R_ref + R_pt1000)
         *   => R_pt1000 = R_ref * adc_val / (adc_max - adc_val)
         *
         * @param raw_adc  Raw ADC count.
         * @return Resistance in Ohms, or 0.0f on error.
         */
        float calculate_resistance(uint32_t raw_adc) const;

        /**
         * @brief Convert a PT1000 resistance to temperature.
         *
         * Uses the linear Callendar-Van Dusen approximation:
         *   T = (R - R0) / (R0 * alpha)
         *
         * @param resistance  Measured resistance in Ohms.
         * @return Temperature in °C, or -273.15f on error.
         */
        float calculate_temperature(float resistance) const;

    private:
        Config  config_;
        uint8_t sensor_id_;
    };

} // namespace PT1000
} // namespace Drivers
