#pragma once

#include <cstddef>
#include <cstdint>

// stm32hal.h may or may not include I2C types depending on configuration.
// Always pull in the I2C stub directly in UNIT_TEST_ENV to guarantee compilation.
#ifndef UNIT_TEST_ENV
#include "stm32h7xx_hal.h"
#else
#include "Drivers/STM32HAL/Simulations/stm32sim_def.hpp"
#include "Drivers/STM32HAL/Simulations/stm32sim_i2c.hpp"
#endif

// ---------------------------------------------------------------------------
// Common driver types
// ---------------------------------------------------------------------------

enum class Status : uint8_t {
    Ok,
    I2cError,
    InvalidChannel,
    MuxNotFound,
    MuxChannelMismatch, // control byte write ACKed, but readback != what was written
    SensorNotFound,
    CrcError,
    InvalidFrame,
    Timeout,
    ProtocolNotDefined
};

template <typename T>
struct Result {
    Status status;
    T      value;
};

// ---------------------------------------------------------------------------
// Driver configuration
// ---------------------------------------------------------------------------

struct SensorConfig {
    uint8_t  mux_address_7bit        = 0x70; // PCA9547 default if A2=A1=A0=0
    uint8_t  mux_channel             = 0;    // 0..7
    uint8_t  sensor_address_7bit     = 0x6C; // PTE7300 7-bit address (excluding CRC)
    bool     use_crc                 = true;
    uint32_t i2c_timeout_ms          = 10;
    float    pressure_full_scale_bar = 16.0f; // configurable per part number
};

struct Pte7300Measurement {
    int16_t pressure_raw;
    int16_t bridge_temperature_raw;
    int16_t status_raw;
};

// ---------------------------------------------------------------------------
// PCA9547 constants  (fully confirmed — datasheet NXP PCA9547 Rev.4)
// ---------------------------------------------------------------------------

static constexpr uint8_t k_pca9547_default_address_7bit = 0x70; // A2=A1=A0=0
static constexpr uint8_t k_pca9547_enable_bit           = 0x08; // control reg bit 3
static constexpr uint8_t k_pca9547_disable_all          = 0x00;
static constexpr uint8_t k_pca9547_max_channel          = 7;

// ch0=0x08, ch1=0x09, ch2=0x0A, ch3=0x0B,
// ch4=0x0C, ch5=0x0D, ch6=0x0E, ch7=0x0F
static constexpr uint8_t pca9547_channel_control_byte(uint8_t channel)
{
    return static_cast<uint8_t>(k_pca9547_enable_bit | (channel & 0x07));
}

// ---------------------------------------------------------------------------
// PTE7300 address constants  (confirmed — Sensata PTE7300 datasheet)
// ---------------------------------------------------------------------------

static constexpr uint8_t k_pte7300_default_address_7bit    = 0x6C;
// 0xDA is listed as "address including CRC" — do NOT use directly with a HAL
// that expects a standard 7-bit address.
static constexpr uint8_t k_pte7300_address_with_crc_listed = 0xDA;

// ---------------------------------------------------------------------------
// PTE7300 protocol constants  (ASSUMED — not in the public product brief)
//
// The PTE7300 commercial datasheet does not publish its I2C register map.
// All values below are derived from common Sensata I2C sensor patterns.
//
// HOW TO DEBUG ON HARDWARE:
//   1. Connect a logic analyzer to the I2C bus.
//   2. Run run_pte7300_hardware_test() and capture the bus traffic.
//   3. Compare the trigger byte (0xAC) and the response bytes against what
//      the sensor actually returns.
//   4. If the sensor NACKs the trigger write, try commenting out the trigger
//      write in read_measurement_raw() and reading directly — some sensors
//      respond to a bare read without a prior command byte.
//   5. If CRC fails, try toggling use_crc=false and log raw bytes.
// ---------------------------------------------------------------------------

// ASSUMED: trigger byte to start an on-demand single-cycle measurement.
// 0xAC is used by several Sensata/Honeywell I2C pressure sensors.
// Alternatives to try if this fails: 0xF3, 0x00, or no trigger at all.
static constexpr uint8_t k_cmd_trigger_measurement = 0xAC;

// ASSUMED: frame returned after a measurement trigger read.
// Format: [press_H, press_L, temp_H, temp_L, status_H, status_L] big-endian
// + 1 CRC byte at the end = 7 bytes total.
// Alternative layout to try if this is wrong:
//   [press_H, press_L, crc, temp_H, temp_L, crc, status_H, status_L, crc]
static constexpr size_t k_measurement_data_bytes = 6;
static constexpr size_t k_measurement_with_crc   = 7; // data + 1 trailing CRC

// ASSUMED: device serial number frame.
// Format: [serial_B3, serial_B2, serial_B1, serial_B0] big-endian + 1 CRC = 5 bytes.
// ASSUMED: same trigger command as measurement; may need a different byte.
static constexpr uint8_t k_cmd_trigger_serial  = 0xAC; // ASSUMED — may differ
static constexpr size_t  k_serial_data_bytes   = 4;
static constexpr size_t  k_serial_with_crc     = 5;

// ASSUMED: CRC-8 parameters (Dallas/Maxim style, common in I2C sensors).
// Polynomial : 0x31
// Init value : 0xFF
// XOR out    : 0x00
// No bit reflection
// If CRC check always fails, try init=0x00 or polynomial=0x07 (SMBUS).
static constexpr uint8_t k_crc8_polynomial = 0x31;
static constexpr uint8_t k_crc8_init       = 0xFF;

// Delay between trigger write and data read (datasheet: response < 1 ms).
// 1 ms provides margin; reduce to 0 if the sensor uses clock-stretching.
static constexpr uint32_t k_pte7300_trigger_delay_ms = 1;

// ---------------------------------------------------------------------------
// Pca9547Mux
// ---------------------------------------------------------------------------

class Pca9547Mux {
public:
    Pca9547Mux(I2C_HandleTypeDef* hi2c, uint8_t address_7bit, uint32_t timeout_ms);

    Status          probe();
    Status          select_channel(uint8_t channel);

    // Same as select_channel(), but reads the control register back
    // afterwards and confirms it matches the byte that was written before
    // returning Ok. Use this before any downstream I2C activity when you
    // need certainty that the switch actually latched the requested
    // channel, not just that the write was ACKed.
    Status          select_channel_verified(uint8_t channel);

    Status          disable_all_channels();
    Result<uint8_t> read_control_register();

private:
    Status write_control_byte(uint8_t byte);

    I2C_HandleTypeDef* hi2c_;
    uint8_t            address_7bit_;
    uint32_t           timeout_ms_;
};

// ---------------------------------------------------------------------------
// SensataPte7300
// ---------------------------------------------------------------------------

class SensataPte7300 {
public:
    SensataPte7300(I2C_HandleTypeDef* hi2c, SensorConfig config);

    Status init();
    Status probe_mux();
    Status probe_sensor();

    Status          select_channel(uint8_t channel);
    Status          disable_mux_channels();
    Result<uint8_t> read_mux_control_register();

    Result<int16_t>            read_pressure_raw();
    Result<int16_t>            read_bridge_temperature_raw();
    Result<int16_t>            read_status_raw();
    Result<uint32_t>           read_device_serial();
    Result<Pte7300Measurement> read_measurement_raw();

private:
    Status ensure_mux_channel_selected();

    // Trigger + read helper: writes trigger_cmd, waits k_pte7300_trigger_delay_ms,
    // then reads frame_len bytes into buf.
    Status trigger_and_read(uint8_t trigger_cmd, uint8_t* buf, size_t frame_len);

    // CRC-8 check: computes CRC over data[0..len-2] and compares with data[len-1].
    bool verify_crc(const uint8_t* data, size_t frame_len);

    Pca9547Mux         mux_;
    SensorConfig       config_;
    I2C_HandleTypeDef* hi2c_;
};
