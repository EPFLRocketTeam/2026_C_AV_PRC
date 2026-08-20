#include "Drivers/KULITE_CTL190/KULITE_CTL190.hpp"
#include "main.h"
#include "../kulite_manual_test.hpp"
#include <cstdio>
#include <cstring>


extern ADC_HandleTypeDef hadc1;  // Kulite (PA4) is wired to ADC1 channel 18 -- see .ioc's PA4.GPIO_Label=Kulite / PA4.Signal=ADCx_INP18

static Ctl190::Config ctl190_cfg;
static Ctl190::Handle ctl190_handle;
static bool g_ctl190_initialized = false;

namespace {
// This sensor's real calibrated sensitivity (0.055 mV/PSI effective, see
// kulite_manual_test config) is much finer than LMT85's -- a single 64-cycle
// sample's residual noise, negligible in degC there, translates into tens
// of PSI of apparent swing here (~0.09 PSI per ADC count). Averaging
// multiple samples per read, same fix as the LMT85 driver, applied here in
// the AdcReadFunction itself so it covers every driver call
// (calibrate_zero/read_pressure/read_raw_mv) that goes through it,
// including the zero calibration itself (previously a single noisy sample).
constexpr int kNumSamples = 128;
} // namespace

/**
 * Fonction ADC pour lire une valeur brute du capteur CTL-190
 * À adapter selon le pin et l' ADC
 *
 * Explicitly (re)configures the channel/sample time here rather than
 * relying on MX_ADC1_Init()'s default -- that default sample time
 * (ADC_SAMPLETIME_1CYCLE_5, the shortest possible) is the same setting
 * that produced ~10 degC of pure ADC noise on the LMT85 driver on this
 * same MCU family; using a longer sample time here preemptively, same
 * fix, same reasoning. Also averages kNumSamples conversions -- see the
 * comment above.
 */
static bool ctl190_adc_read(uint32_t* raw_value) {
    if (raw_value == nullptr) {
        return false;
    }

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel                = ADC_CHANNEL_18; // PA4/Kulite, ADC1 -- see .ioc
    sConfig.Rank                   = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime           = ADC_SAMPLETIME_64CYCLES_5;
    sConfig.SingleDiff             = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber           = ADC_OFFSET_NONE;
    sConfig.Offset                 = 0;
    sConfig.OffsetSignedSaturation = DISABLE;

    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) {
        return false;
    }

    uint64_t sum = 0;
    for (int i = 0; i < kNumSamples; ++i) {
        if (HAL_ADC_Start(&hadc1) != HAL_OK) {
            return false;
        }

        if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK) {
            HAL_ADC_Stop(&hadc1);
            return false;
        }

        sum += HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
    }

    *raw_value = static_cast<uint32_t>(sum / kNumSamples);

    return true;
}


/**
 * Convertir un status code en string pour affichage
 */
static const char* status_to_string(Ctl190::Status status) {
    switch (status) {
        case Ctl190::Status::OK:
            return "OK";
        case Ctl190::Status::ERROR_NOT_INITIALIZED:
            return "NOT_INITIALIZED";
        case Ctl190::Status::ERROR_ADC_READ_FAILED:
            return "ADC_READ_FAILED";
        case Ctl190::Status::ERROR_OUT_OF_RANGE:
            return "OUT_OF_RANGE";
        default:
            return "UNKNOWN";
    }
}

/**
 * Afficher la configuration courante
 */
static void print_configuration() {
    printf("\r\n--- Configuration CTL-190 ---\r\n");
    printf("  ADC Resolution   : %d bits\r\n", ctl190_cfg.adc_resolution);
    printf("  ADC Vref         : %.2f V\r\n", ctl190_cfg.adc_vref);
    printf("  Amplifier Gain   : %.2f\r\n", ctl190_cfg.amp_gain);
    printf("  FSO (nominal)    : %.2f mV\r\n", ctl190_cfg.fso_mv);
    printf("  Pressure Rated   : %.2f %s\r\n",
           ctl190_cfg.pressure_max,
           ctl190_cfg.unit == Ctl190::PressureUnit::PSI ? "PSI" : "BAR");
    printf("  Max Overpressure : %.2f %s\r\n",
           ctl190_cfg.max_overpressure,
           ctl190_cfg.unit == Ctl190::PressureUnit::PSI ? "PSI" : "BAR");
    printf("  Sensitivity      : %.4f mV/%s @ %.1f V excitation\r\n",
           ctl190_cfg.sensitivity_mv_per_unit,
           ctl190_cfg.unit == Ctl190::PressureUnit::PSI ? "PSI" : "BAR",
           ctl190_cfg.calibration_excitation_v);
    printf("  Actual Excitation: %.1f V\r\n", ctl190_cfg.actual_excitation_v);
    printf("  Zero Offset      : %.2f mV\r\n", ctl190_handle.zero_offset_mv);
    printf("\r\n");
}

// Original phased test (Phase 1-9: config, sanity check, init, zero-cal,
// single raw/pressure reads, 30s timed stream, summary, pass/fail verdict).
// Kept for reference, not deleted -- superseded below by a version that
// loops forever printing readings instead of running once for a fixed
// 30s and returning, same shape as the LMT85/PT1000 manual tests.
/*
int manual_test_ctl190_phased() {
    printf("\r\n");

    // Phase 1: Configuration
    printf("\r\n[Phase 1] Configuration ............. ");
    ctl190_cfg.adc_read       = ctl190_adc_read;
    ctl190_cfg.adc_resolution = 16;           // ADC1 configured for 16-bit on STM32H7
    ctl190_cfg.adc_vref       = 3.3f;         // 3.3V reference
    ctl190_cfg.amp_gain       = 10.0f;        // INA128 with gain 10
    ctl190_cfg.fso_mv         = 100.0f;       // CTL-190 FSO
    ctl190_cfg.pressure_max   = 300.0f;       // 0-300 PSI
    ctl190_cfg.unit           = Ctl190::PressureUnit::PSI;
    printf("OK\r\n");

    // Phase 2: ADC Sanity Check
    printf("[Phase 2] ADC sanity check ......... ");
    uint32_t raw_test = 0;
    if (!ctl190_adc_read(&raw_test)) {
        printf("FAIL (ADC read error)\r\n");
        return -1;
    }
    printf("Raw = %lu\r\n", raw_test);

    // Phase 3: Driver Initialization
    printf("[Phase 3] Driver init ............. ");
    Ctl190::Status status = Ctl190::init(&ctl190_handle, ctl190_cfg);
    if (status != Ctl190::Status::OK) {
        printf("FAIL (%s)\r\n", status_to_string(status));
        return -1;
    }
    g_ctl190_initialized = true;
    printf("OK\r\n");

    print_configuration();

    // Phase 4: Zero Calibration
    printf("[Phase 4] Zero calibration ........ ");
    printf("(Keep sensor at atmospheric pressure)\r\n");
    printf("          Waiting 2 seconds ... ");
    fflush(stdout);

    HAL_Delay(2000);

    status = Ctl190::calibrate_zero(&ctl190_handle);
    if (status != Ctl190::Status::OK) {
        printf("FAIL (%s)\r\n", status_to_string(status));
        return -1;
    }
    printf("OK\r\n");
    printf("          Zero offset calibrated: %.2f mV\r\n", ctl190_handle.zero_offset_mv);

    // Phase 5: Raw Voltage Test
    printf("\r\n[Phase 5] Raw voltage reading ..... ");
    float raw_mv = 0.0f;
    status = Ctl190::read_raw_mv(&ctl190_handle, &raw_mv);
    if (status != Ctl190::Status::OK) {
        printf("FAIL (%s)\r\n", status_to_string(status));
        return -1;
    }
    printf("OK (%.2f mV)\r\n", raw_mv);

    // Phase 6: Single Pressure Reading
    printf("[Phase 6] Single pressure read .... ");
    float pressure = 0.0f;
    status = Ctl190::read_pressure(&ctl190_handle, &pressure);
    printf("%s (%.2f PSI)\r\n", status_to_string(status), pressure);

    if (status == Ctl190::Status::ERROR_OUT_OF_RANGE) {
        printf("          WARNING: Out of range (expected if pressure != 0)\r\n");
    }

    // Phase 7: Continuous Streaming
    printf("\r\n[Phase 7] Streaming at 10 Hz (30 seconds)\r\n");
    printf("          Apply pressure variations and observe readings\r\n");
    printf("          Format: [Count] Raw(ADC) | Voltage(mV) | Pressure(PSI) | Status\r\n");
    printf("          ─────────────────────────────────────────────────────────────────\r\n");

    uint32_t start_time = HAL_GetTick();
    uint32_t read_count = 0;
    float min_pressure = 1e6f, max_pressure = -1e6f;
    int out_of_range_count = 0;

    while ((HAL_GetTick() - start_time) < 30000u) {  // 30 secondes
        // Lire à ~10 Hz (toutes les 100 ms)
        HAL_Delay(100);

        // Lire tension brute
        status = Ctl190::read_raw_mv(&ctl190_handle, &raw_mv);
        if (status != Ctl190::Status::OK) {
            continue;
        }

        // Lire pression
        pressure = 0.0f;
        status = Ctl190::read_pressure(&ctl190_handle, &pressure);

        read_count++;

        // Afficher tous les 5 readings (~500 ms)
        if (read_count % 5 == 0) {
            printf("          [%3lu] ADC Raw: %4lu | Voltage: %6.2f mV | Pressure: %7.2f PSI | %s\r\n",
                   read_count,
                   raw_test,  // Note: raw_test garde la dernière valeur
                   raw_mv,
                   pressure,
                   status_to_string(status));
        }

        // Statistiques
        if (status == Ctl190::Status::OK) {
            if (pressure < min_pressure) min_pressure = pressure;
            if (pressure > max_pressure) max_pressure = pressure;
        } else if (status == Ctl190::Status::ERROR_OUT_OF_RANGE) {
            out_of_range_count++;
        }

        // Relire l'ADC pour la prochaine itération
        ctl190_adc_read(&raw_test);
    }

    // Phase 8: Summary
    printf("\r\n[Phase 8] Test Summary\r\n");
    printf("          ─────────────────────────────────────────────────────────────────\r\n");
    printf("          Total readings    : %lu\r\n", read_count);
    printf("          Min pressure      : %.2f PSI\r\n", min_pressure < 1e6f ? min_pressure : 0.0f);
    printf("          Max pressure      : %.2f PSI\r\n", max_pressure > -1e6f ? max_pressure : 0.0f);
    printf("          Out of range      : %d\r\n", out_of_range_count);
    printf("          Success rate      : %.1f %%\r\n",
           100.0f * (read_count - out_of_range_count) / read_count);

    // Phase 9: Final Status
    printf("\r\n[Phase 9] Final Status\r\n");
    printf("          ─────────────────────────────────────────────────────────────────\r\n");

    if (g_ctl190_initialized && read_count > 0 && out_of_range_count == 0) {
        printf("          ✓ CTL-190 Driver: OPERATIONAL\r\n");
        printf("          ✓ ADC: WORKING\r\n");
        printf("          ✓ Calibration: VALID\r\n");
        printf("          ✓ All readings: IN RANGE\r\n");
        printf("\r\n          TEST PASSED ✓\r\n");
        return 0;
    } else if (g_ctl190_initialized && read_count > 0) {
        printf("          ✓ CTL-190 Driver: OPERATIONAL\r\n");
        printf("          ⚠ Some readings out of range (expected if pressure applied)\r\n");
        printf("\r\n          TEST PASSED WITH WARNINGS ⚠\r\n");
        return 0;
    } else {
        printf("          ✗ CTL-190 Driver: FAILED\r\n");
        printf("\r\n          TEST FAILED ✗\r\n");
        return -1;
    }
}
*/

/**
 * Manual hardware test for the CTL-190. One-time config/init/zero-cal,
 * then loops forever printing raw ADC + voltage + pressure once per
 * second -- same shape as the LMT85/PT1000 manual tests, so main.c can
 * swap between them the same way. Runs until reset; does not return in
 * practice (only returns -1 early if init/calibration fails).
 */
int manual_test_ctl190() {
    printf("\r\n[CTL190] Manual test starting...\r\n");

    ctl190_cfg.adc_read       = ctl190_adc_read;
    ctl190_cfg.adc_resolution = 16;           // ADC3 configured for 16-bit on STM32H7
    ctl190_cfg.adc_vref       = 3.3f;         // 3.3V reference
    ctl190_cfg.amp_gain       = 10.0f;        // INA128 with gain 10
    ctl190_cfg.fso_mv         = 100.0f;       // CTL-190 family-nominal FSO, display only
    ctl190_cfg.pressure_max   = 2000.0f;      // rated pressure, per this unit's cal certificate
    ctl190_cfg.max_overpressure         = 3000.0f; // proof/burst rating, per cal certificate
    ctl190_cfg.sensitivity_mv_per_unit  = 0.05f;   // as-calibrated, per cal certificate
    ctl190_cfg.calibration_excitation_v = 10.0f;   // excitation the sensitivity above was measured at
    ctl190_cfg.actual_excitation_v      = 11.0f;   // excitation actually driving this circuit
    ctl190_cfg.unit           = Ctl190::PressureUnit::PSI;

    Ctl190::Status status = Ctl190::init(&ctl190_handle, ctl190_cfg);
    if (status != Ctl190::Status::OK) {
        printf("[CTL190] ERROR: init failed (%s)\r\n", status_to_string(status));
        return -1;
    }
    g_ctl190_initialized = true;

    print_configuration();

    printf("[CTL190] Zero calibration -- keep sensor at atmospheric pressure...\r\n");
    HAL_Delay(2000);

    status = Ctl190::calibrate_zero(&ctl190_handle);
    if (status != Ctl190::Status::OK) {
        printf("[CTL190] ERROR: zero calibration failed (%s)\r\n", status_to_string(status));
        return -1;
    }
    printf("[CTL190] Zero offset: %.2f mV\r\n", ctl190_handle.zero_offset_mv);

    printf("[CTL190] Streaming -- Raw(ADC) | Voltage(mV) | Pressure(PSI / bar) | Status\r\n");

    // Calibration data (sensitivity, rated/overpressure) is in PSI, per the
    // cal certificate -- kept as-is, not reworked into bar, to avoid
    // touching the carefully-derived sensitivity/excitation-correction
    // math. Bar is a display-only conversion of the PSI result.
    constexpr float kPsiPerBar = 14.5037738f;

    while (1) {
        uint32_t raw = 0;
        float raw_mv = 0.0f;
        float pressure_psi = 0.0f;

        ctl190_adc_read(&raw);
        Ctl190::read_raw_mv(&ctl190_handle, &raw_mv);
        Ctl190::Status p_status = Ctl190::read_pressure(&ctl190_handle, &pressure_psi);
        const float pressure_bar = pressure_psi / kPsiPerBar;

        printf("[CTL190] ADC: %lu | V: %.2f mV | P: %.2f PSI (%.3f bar) | %s\r\n",
               (unsigned long)raw, raw_mv, pressure_psi, pressure_bar, status_to_string(p_status));

        HAL_Delay(1000);
    }

    return 0; // unreachable
}

/**
 * Fonction pour relancer le test manuellement
 */
int ctl190_test_reset() {
    printf("\r\n[Reset] Reinitializing CTL-190 driver ...\r\n");
    g_ctl190_initialized = false;
    memset(&ctl190_handle, 0, sizeof(ctl190_handle));
    return manual_test_ctl190();
}

/**
 * Fonction pour afficher l'état courant du capteur
 */
int ctl190_test_get_status() {
    if (!g_ctl190_initialized) {
        printf("CTL-190 not initialized\r\n");
        return -1;
    }

    float raw_mv = 0.0f;
    float pressure = 0.0f;

    Ctl190::Status status = Ctl190::read_raw_mv(&ctl190_handle, &raw_mv);
    if (status != Ctl190::Status::OK) {
        printf("Error reading raw voltage: %s\r\n", status_to_string(status));
        return -1;
    }

    status = Ctl190::read_pressure(&ctl190_handle, &pressure);

    printf("CTL-190 Status:\r\n");
    printf("  Raw Voltage  : %.2f mV\r\n", raw_mv);
    printf("  Pressure     : %.2f PSI\r\n", pressure);
    printf("  Status       : %s\r\n", status_to_string(status));
    printf("  Initialized  : %s\r\n", g_ctl190_initialized ? "Yes" : "No");

    return 0;
}
