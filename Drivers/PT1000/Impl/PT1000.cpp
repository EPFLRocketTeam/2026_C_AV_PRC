#include "../PT1000.hpp"

namespace Drivers {
namespace PT1000 {

PT1000Driver::PT1000Driver(Config config, uint8_t sensor_id)
    : config_(config), sensor_id_(sensor_id) {}

bool PT1000Driver::init() {
    if (config_.hadc == nullptr) {
        return false;
    }
    // The ADC peripheral is typically initialized by STM32CubeMX-generated code
    // (MX_ADCx_Init). Nothing extra is needed here unless per-channel calibration
    // is required in the future.
    return true;
}

bool PT1000Driver::read(PT1000Data& out) {
    out.valid     = false;
    out.sensor_id = sensor_id_;
    out.raw_adc   = 0;

    if (config_.hadc == nullptr) {
        return false;
    }

    // Start ADC conversion (blocking, single channel)
    if (HAL_ADC_Start(config_.hadc) != HAL_OK) {
        return false;
    }

    // Wait for end of conversion (timeout 100 ms)
    if (HAL_ADC_PollForConversion(config_.hadc, 100) != HAL_OK) {
        HAL_ADC_Stop(config_.hadc);
        return false;
    }

    out.raw_adc = HAL_ADC_GetValue(config_.hadc);
    HAL_ADC_Stop(config_.hadc);

    out.resistance  = calculate_resistance(out.raw_adc);
    out.temperature = calculate_temperature(out.resistance);
    out.valid       = (out.resistance > 0.0f);

    return out.valid;
}

float PT1000Driver::calculate_resistance(uint32_t raw_adc) const {
    // Guard: adc value at 0 or at/above max means shorted or open sensor
    if (raw_adc == 0 || raw_adc >= config_.adc_max) {
        return 0.0f;
    }

    // Voltage divider: R_ref on high side, PT1000 on low side (to GND)
    //   V_adc / V_ref = R_pt / (R_ref + R_pt)
    //   => R_pt = R_ref * adc / (adc_max - adc)
    float ratio = static_cast<float>(raw_adc) / static_cast<float>(config_.adc_max - raw_adc);
    return config_.r_ref * ratio;
}

float PT1000Driver::calculate_temperature(float resistance) const {
    // Invalid resistance → return absolute zero as error sentinel
    if (resistance <= 0.0f) {
        return -273.15f;
    }

    // Linear Callendar-Van Dusen approximation
    //   R(T) = R0 * (1 + alpha * T)
    //   => T  = (R - R0) / (R0 * alpha)
    return (resistance - R0) / (R0 * ALPHA);
}

} // namespace PT1000
} // namespace Drivers