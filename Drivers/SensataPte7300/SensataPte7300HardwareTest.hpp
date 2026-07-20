#ifndef SENSATA_TESTS_HARDWARE_MAIN_TEST_H_
#define SENSATA_TESTS_HARDWARE_MAIN_TEST_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Manual hardware test for PT1000 temperature sensors.
 *
 * Reads all 4 PT1000 channels in a loop and prints resistance + temperature
 * via printf (redirected to UART/SWO). Runs forever until the board is reset.
 *
 * @return 0 (never returns in practice).
 */
void run_pte7300_hardware_test();

/**
 * @brief One-shot PTE7300 data read + print: pressure (bar) and
 * temperature (C) only -- no mux probing, channel walk, serial number, or
 * hints. Unlike run_pte7300_hardware_test(), this does not loop or block:
 * call it repeatedly from the main loop (same pattern as
 * FDC1004_ManualTest()) to get a continuous data stream.
 */
void pte7300_print_data();

/**
 * @brief Continuous I2C bus scanner across every PCA9547 mux channel.
 *
 * Selects each mux channel (0..7) in turn and probes every 7-bit address
 * in the standard scan range (0x03-0x77) with HAL_I2C_IsDeviceReady().
 * Any address that ACKs is logged via printf (redirected to UART/SWO).
 * After a full 8-channel pass, waits 500 ms and repeats forever.
 *
 * Note: the mux itself (default 0x70) will ACK on every channel since it
 * sits on the main bus segment, not behind a channel — that's expected.
 *
 * @return never returns in practice.
 */
void run_pte7300_i2c_scanner();

/**
 * @brief Bursts raw I2C writes on mux channel 0 for scope probing.
 *
 * At the start of every cycle, selects mux channel 0 and reads the control
 * register back to confirm the switch actually latched (not just that the
 * write was ACKed) before writing a 1-byte trigger command to the sensor
 * address as fast as the bus allows for 500 ms ("burst"), followed by
 * 500 ms of bus silence ("idle"), forever. If the verified select ever
 * fails, that is logged instead of silently hammering a disconnected bus.
 *
 * Attach an oscilloscope to SD0/SC0 (the mux's channel-0 output, i.e. the
 * wires going to the sensor) and use the burst/idle transition as a scope
 * trigger to inspect signal integrity downstream of the mux: idle-high
 * level, rise/fall times (weak or missing pull-ups show up as slow edges),
 * clean START / address+R/W / ACK-or-NACK / STOP framing, and whether the
 * bus is stuck low.
 *
 * @return never returns in practice.
 */
void run_pte7300_channel0_scope_probe();

#ifdef __cplusplus
}
#endif

#endif /* SENSATA_TESTS_HARDWARE_MAIN_TEST_H_ */
