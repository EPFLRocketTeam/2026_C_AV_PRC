#ifndef DRIVER_CTL190_H
#define DRIVER_CTL190_H

#include <cstdint>

/**
 * @file Ctl190.hpp
 * @brief Driver for the Kulite CTL-190 Cryogenic Pressure Transducer
 *
 * The CTL-190 is an analog pressure sensor using a fully active
 * four-arm Wheatstone bridge (silicon-on-silicon). It outputs
 * 0-100 mV (nominal) at 10 VDC excitation, proportional to the
 * measured pressure.
 *
 * The analog signal is amplified by an instrumentation amplifier
 * (INA128) before being read by the STM32 internal ADC.
 *
 * This driver converts the raw ADC reading into a pressure value
 * in the configured unit (bar or psi).
 */

namespace Ctl190 {

    /* ---- CTL-190 Datasheet Constants ---- */
    constexpr float FSO_MV                = 100.0f;   /**< Full Scale Output: 100 mV nominal    */
    constexpr float EXCITATION_V          = 10.0f;    /**< Rated excitation: 10 VDC              */
    constexpr float MAX_EXCITATION_V      = 12.0f;    /**< Maximum excitation: 12 VDC            */
    constexpr float INPUT_IMPEDANCE_OHM   = 1000.0f;  /**< Input impedance: 1000 Ohms min        */
    constexpr float OUTPUT_IMPEDANCE_OHM  = 1000.0f;  /**< Output impedance: 1000 Ohms nominal   */
    constexpr float RESIDUAL_UNBALANCE_MV = 5.0f;     /**< Residual unbalance: +/-5 mV typical   */
    constexpr float NONLINEARITY_PCT_TYP  = 0.1f;     /**< Non-linearity: +/-0.1% FSO BFSL typ.  */
    constexpr float NONLINEARITY_PCT_MAX  = 0.5f;     /**< Non-linearity: +/-0.5% FSO maximum    */

    /**
     * Status codes returned by driver functions
     */
    enum class Status {
        OK,
        ERROR_NOT_INITIALIZED,
        ERROR_ADC_READ_FAILED,
        ERROR_OUT_OF_RANGE
    };

    /**
     * Pressure unit used for the output
     */
    enum class PressureUnit {
        BAR,
        PSI
    };

    /**
     * Function pointer type for reading a raw ADC value.
     *
     * This abstracts the ADC hardware so that:
     *   - On real hardware, you provide a function calling HAL_ADC
     *   - In unit tests, you provide a mock returning controlled values
     *
     * @param[out] raw_value  Pointer where the raw ADC value is stored
     * @return true if the read succeeded, false otherwise
     */
    typedef bool (*AdcReadFunction)(uint32_t* raw_value);

    /**
     * Configuration for the CTL-190 driver
     *
     * All electrical parameters of the signal chain must be provided
     * so the driver can convert raw ADC counts to pressure.
     */
    struct Config {
        /* --- ADC parameters --- */
        AdcReadFunction adc_read;       /**< Function to read a raw ADC value       */
        uint8_t         adc_resolution; /**< ADC resolution in bits (e.g. 12, 16)   */
        float           adc_vref;       /**< ADC reference voltage in volts (e.g. 3.3) */

        /* --- Amplifier parameters --- */
        float           amp_gain;       /**< Gain of the INA128 (= 1 + 50k/R_G)    */

        /* --- Sensor parameters --- */
        float           fso_mv;         /**< Full Scale Output in mV (nominal 100)  */
        float           pressure_max;   /**< Full scale pressure (sensor range max) */
        PressureUnit    unit;           /**< Output pressure unit (BAR or PSI)      */
    };

    /**
     * Handle for a CTL-190 sensor instance
     */
    struct Handle {
        Config config;
        float  zero_offset_mv;  /**< Zero offset in mV (calibrated at zero pressure) */
        bool   initialized;
    };

    /**
     * Initialize the CTL-190 driver
     *
     * @param[out] handle  Driver handle to initialize
     * @param[in]  config  Configuration parameters
     * @return Status::OK on success
     */
    Status init(Handle* handle, const Config& config);

    /**
     * Calibrate the zero offset at atmospheric / zero pressure
     *
     * Call this function when the sensor is at a known zero-pressure
     * state. It reads the current ADC value and stores it as the
     * zero offset, so subsequent readings are corrected.
     *
     * @param[in,out] handle  Initialized driver handle
     * @return Status::OK on success
     */
    Status calibrate_zero(Handle* handle);

    /**
     * Read the current pressure
     *
     * Reads the ADC, compensates for zero offset and amplifier gain,
     * and converts to pressure in the configured unit.
     *
     * @param[in]  handle    Initialized driver handle
     * @param[out] pressure  Resulting pressure value
     * @return Status::OK on success
     */
    Status read_pressure(const Handle* handle, float* pressure);

    /**
     * Read the raw sensor voltage in millivolts (before gain)
     *
     * Useful for debugging and diagnostics.
     *
     * @param[in]  handle  Initialized driver handle
     * @param[out] mv      Sensor voltage in millivolts
     * @return Status::OK on success
     */
    Status read_raw_mv(const Handle* handle, float* mv);

} // namespace Ctl190

#endif /* DRIVER_CTL190_H */
