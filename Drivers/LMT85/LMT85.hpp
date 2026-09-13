#pragma once

#include "stm32h7xx_hal.h"
#include <cstdint>
#include "Drivers/LMT85/Types.hpp"

namespace Drivers {
namespace LMT85 {

    // ======================================================================
    // CONFIGURATION
    // ======================================================================

    constexpr uint32_t LMT85_1SAMPLE   = 0;
    constexpr uint32_t LMT85_2SAMPLE   = 1;
    constexpr uint32_t LMT85_4SAMPLE   = 2;
    constexpr uint32_t LMT85_8SAMPLE   = 3;
    constexpr uint32_t LMT85_16SAMPLE  = 4;
    constexpr uint32_t LMT85_32SAMPLE  = 5;
    constexpr uint32_t LMT85_64SAMPLE  = 6;
    constexpr uint32_t LMT85_128SAMPLE = 7;

    struct Config {
        ADC_HandleTypeDef* hadc;        // Pointer to the ADC handle
        uint32_t           adc_channel; // ADC channel number
        uint32_t           adc_max;     // Max ADC value (e.g. 4095 for 12-bit, 65535 for 16-bit)
        float              adc_vref_mv; // ADC reference voltage, in mV (e.g. 3300.0f for 3.3V)

        uint32_t sampling_time  = ADC_SAMPLETIME_64CYCLES_5;
        uint32_t num_samples_l2 = LMT85_16SAMPLE; // (1 << num_samples_l2) samples

        // LMT85 quadratic transfer function (TI datasheet SNIS168E, Section
        // 8.3.1, Equation 1), fit around a reference point (t0_c, v0_mv):
        //   V(mV) = v0_mv + a_mv_per_c * (T - t0_c) + b_mv_per_c2 * (T - t0_c)^2
        //
        // Defaults below are the datasheet's own fit constants (not
        // placeholders): V = 1324.0 - 8.194*(T-30) - 0.00262*(T-30)^2, valid
        // across the full -50°C to 150°C range with the datasheet's
        // Table 3 accuracy (typ ±0.4°C at 20-150°C/1.8-5.5V, up to ±2.7°C
        // max at range extremes / low VDD). Per TI, accuracy degrades
        // slightly further from t0_c=30°C.
        float v0_mv       = 1324.0f;  // output voltage (mV) at t0_c
        float t0_c        = 30.0f;    // reference temperature (°C)
        float a_mv_per_c  = -8.194f;  // linear term (mV/°C)
        float b_mv_per_c2 = -0.00262f; // quadratic term (mV/°C^2)
    };

    // ======================================================================
    // DRIVER CLASS
    // ======================================================================

    class LMT85Driver {
    public:
        LMT85Driver () = default;
        LMT85Driver(Config config);

        // The ADC peripheral is initialized by CubeMX-generated code
        // (MX_ADCx_Init); this just validates the config.
        bool init();

        // Blocking ADC read + conversion to temperature.
        LMT85Status read(LMT85Data& out);

        // Raw ADC count -> output voltage, in mV.
        float calculate_voltage_mv(uint32_t raw_adc) const;

        // Output voltage (mV) -> temperature (°C), by solving the quadratic
        // transfer function in Config for T (datasheet Equation 2).
        float calculate_temperature(float voltage_mv) const;

    private:
        Config config_;
    };

} // namespace LMT85
} // namespace Drivers
