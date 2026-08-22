#ifndef PT1000_TESTS_HARDWARE_MAIN_TEST_PT1000_H_
#define PT1000_TESTS_HARDWARE_MAIN_TEST_PT1000_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manual hardware test for PT1000 temperature sensors.
 *
 * Reads all 4 PT1000 channels in a loop and prints resistance + temperature
 * via app_printf (redirected to UART/SWO). Runs forever until the board is reset.
 *
 * @return 0 (never returns in practice).
 */
int manual_test_pt1000();

#ifdef __cplusplus
}
#endif

#endif /* PT1000_TESTS_HARDWARE_MAIN_TEST_PT1000_H_ */
