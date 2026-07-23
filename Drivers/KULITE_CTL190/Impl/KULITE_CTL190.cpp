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

    if (config.sensitivity_mv_per_unit <= 0.0f || config.calibration_excitation_v <= 0.0f ||
        config.actual_excitation_v <= 0.0f || config.max_overpressure <= 0.0f) {
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

    /* Convert millivolts to pressure using this sensor's real calibrated
     * sensitivity, not a generic fso_mv/pressure_max ratio -- a given
     * unit's actual sensitivity can differ substantially from the
     * family-nominal FSO/range ratio (confirmed on this unit: 0.05 mV/PSI
     * measured vs. ~0.33 mV/PSI implied by nominal 100mV FSO / 300 PSI).
     *
     * Bridge output is directly proportional to excitation voltage, so the
     * as-calibrated sensitivity (measured at calibration_excitation_v) is
     * scaled by (actual_excitation_v / calibration_excitation_v) to get the
     * effective sensitivity for the excitation actually driving this
     * circuit -- e.g. calibrated at 10V but run at 11V reads ~10% high
     * unless corrected.
     */
    float effective_sensitivity = cfg.sensitivity_mv_per_unit *
                                   (cfg.actual_excitation_v / cfg.calibration_excitation_v);
    float p = sensor_mv / effective_sensitivity;

    /* Overpressure detection: flag readings above this sensor's real
     * calibrated max_overpressure (proof/burst rating, e.g. 3000 PSI on a
     * 2000 PSI rated unit -- the CTL-190 datasheet's "1.5x rated above 500
     * PSI" overpressure rule), not an arbitrary margin on pressure_max.
     */
    if (p > cfg.max_overpressure || p < -cfg.pressure_max * 0.1f) {
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

}
