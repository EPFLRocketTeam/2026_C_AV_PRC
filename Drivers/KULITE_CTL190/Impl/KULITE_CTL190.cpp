#include "Drivers/KULITE_CTL190/KULITE_CTL190.hpp"

namespace Ctl190 {

/**
 * Internal helper: convert a raw ADC value to the sensor voltage
 * in millivolts (i.e. the voltage at the CTL-190 output, before
 * the instrumentation amplifier).
 *
 *   v_adc    = (raw / (2^resolution - 1)) * v_ref
 *   v_sensor = v_adc / amp_gain
 *   mv       = v_sensor * 1000
 */
static float adc_raw_to_sensor_mv(const Config& cfg, uint32_t raw) {
    float max_counts = static_cast<float>((1U << cfg.adc_resolution) - 1U);
    float v_adc      = (static_cast<float>(raw) / max_counts) * cfg.adc_vref;
    float v_sensor   = v_adc / cfg.amp_gain;
    return v_sensor * 1000.0f;
}

Status init(Handle* handle, const Config& config) {
    if (handle == nullptr || config.adc_read == nullptr) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    if (config.adc_resolution == 0 || config.adc_resolution > 32) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    if (config.amp_gain <= 0.0f || config.fso_mv <= 0.0f || config.pressure_max <= 0.0f) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    handle->config         = config;
    handle->zero_offset_mv = 0.0f;
    handle->initialized    = true;

    return Status::OK;
}

Status calibrate_zero(Handle* handle) {
    if (handle == nullptr || !handle->initialized) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    uint32_t raw = 0;
    if (!handle->config.adc_read(&raw)) {
        return Status::ERROR_ADC_READ_FAILED;
    }

    handle->zero_offset_mv = adc_raw_to_sensor_mv(handle->config, raw);
    return Status::OK;
}

Status read_pressure(const Handle* handle, float* pressure) {
    if (handle == nullptr || !handle->initialized) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    if (pressure == nullptr) {
        return Status::ERROR_ADC_READ_FAILED;
    }

    uint32_t raw = 0;
    if (!handle->config.adc_read(&raw)) {
        return Status::ERROR_ADC_READ_FAILED;
    }

    const Config& cfg = handle->config;

    /* Convert ADC raw value to sensor millivolts */
    float sensor_mv = adc_raw_to_sensor_mv(cfg, raw);

    /* Subtract zero offset (compensates Residual Unbalance) */
    sensor_mv -= handle->zero_offset_mv;

    /* Convert millivolts to pressure:
     *   pressure = (sensor_mv / fso_mv) * pressure_max
     *
     * This is a linear relationship because the CTL-190
     * Wheatstone bridge output is proportional to pressure.
     */
    float p = (sensor_mv / cfg.fso_mv) * cfg.pressure_max;

    /* Overpressure detection:
     * The CTL-190 datasheet specifies max 2x rated pressure (up to 500 PSI)
     * and 1.5x above 500 PSI. We flag readings beyond 1.1x full scale
     * as out of range to provide an early warning margin.
     */
    if (p > cfg.pressure_max * 1.1f || p < -cfg.pressure_max * 0.1f) {
        *pressure = p; /* Still store the value for diagnostics */
        return Status::ERROR_OUT_OF_RANGE;
    }

    *pressure = p;
    return Status::OK;
}

Status read_raw_mv(const Handle* handle, float* mv) {
    if (handle == nullptr || !handle->initialized) {
        return Status::ERROR_NOT_INITIALIZED;
    }

    if (mv == nullptr) {
        return Status::ERROR_ADC_READ_FAILED;
    }

    uint32_t raw = 0;
    if (!handle->config.adc_read(&raw)) {
        return Status::ERROR_ADC_READ_FAILED;
    }

    *mv = adc_raw_to_sensor_mv(handle->config, raw);
    return Status::OK;
}

} // namespace Ctl190