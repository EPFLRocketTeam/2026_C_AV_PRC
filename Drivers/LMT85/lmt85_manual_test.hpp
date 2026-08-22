#ifndef LMT85_MANUAL_TEST_H
#define LMT85_MANUAL_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manual hardware test for the LMT85 temperature sensor (Vtemp, PB1).
 *
 * Reads the sensor in a loop and prints raw ADC, voltage, and temperature
 * via app_printf. Runs forever until the board is reset.
 *
 * @return 0 (never returns in practice).
 */
int manual_test_lmt85();

#ifdef __cplusplus
}
#endif

#endif /* LMT85_MANUAL_TEST_H */
