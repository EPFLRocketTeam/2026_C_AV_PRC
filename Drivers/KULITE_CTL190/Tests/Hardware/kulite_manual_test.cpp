#include "Drivers/KULITE_CTL190/KULITE_CTL190.hpp"
#include "main.h"
#include "kulite_manual_test.hpp"
#include <cstdio>
#include <cstring>


extern ADC_HandleTypeDef hadc1;  // À adapter selon le STM32

static Ctl190::Config ctl190_cfg;
static Ctl190::Handle ctl190_handle;
static bool g_ctl190_initialized = false;

/**
 * Fonction ADC pour lire une valeur brute du capteur CTL-190
 * À adapter selon le pin et l' ADC
 */
static bool ctl190_adc_read(uint32_t* raw_value) {
    if (raw_value == nullptr) {
        return false;
    }

    // Lancer la conversion ADC
    if (HAL_ADC_Start(&hadc1) != HAL_OK) {
        return false;
    }

    // Attendre la fin de conversion (timeout 1000 ms)
    if (HAL_ADC_PollForConversion(&hadc1, 1000) != HAL_OK) {
        return false;
    }

    // Lire la valeur brute
    *raw_value = HAL_ADC_GetValue(&hadc1);

    // Arrêter la conversion
    HAL_ADC_Stop(&hadc1);

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
    printf("  ADC Resolution : %d bits\r\n", ctl190_cfg.adc_resolution);
    printf("  ADC Vref       : %.2f V\r\n", ctl190_cfg.adc_vref);
    printf("  Amplifier Gain : %.2f\r\n", ctl190_cfg.amp_gain);
    printf("  FSO            : %.2f mV\r\n", ctl190_cfg.fso_mv);
    printf("  Pressure Max   : %.2f %s\r\n", 
           ctl190_cfg.pressure_max,
           ctl190_cfg.unit == Ctl190::PressureUnit::PSI ? "PSI" : "BAR");
    printf("  Zero Offset    : %.2f mV\r\n", ctl190_handle.zero_offset_mv);
    printf("\r\n");
}

int manual_test_ctl190() {
    printf("\r\n");

    // Phase 1: Configuration
    printf("\r\n[Phase 1] Configuration ............. ");
    ctl190_cfg.adc_read       = ctl190_adc_read;
    ctl190_cfg.adc_resolution = 12;           // STM32F4 = 12-bit
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