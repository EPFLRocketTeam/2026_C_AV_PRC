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

// Identifies which internal step of read_measurement_raw()/read_device_serial()
// a non-Ok Result came from -- a single I2cError doesn't say whether the mux
// select, the START write, or one specific register read is the failing one.
enum class Pte7300Step : uint8_t {
    None,
    MuxSelect,
    WriteStart,
    ReadDspT,
    ReadDspS,
    ReadStatus,
    ReadSerial
};

// ---------------------------------------------------------------------------
// Driver configuration
// ---------------------------------------------------------------------------

struct SensorConfig {
    uint8_t  mux_address_7bit        = 0x70; // PCA9547 default if A2=A1=A0=0
    uint8_t  mux_channel             = 0;    // 0..7
    uint8_t  sensor_address_7bit     = 0x6C; // PTE7300 7-bit address (excluding CRC)
    // CRC-mode framing (address|1, CRC4 header + CRC8 payload) is not yet
    // ported -- read_measurement_raw()/read_device_serial() return
    // Status::ProtocolNotDefined if this is true. Default false so the
    // confirmed no-CRC register protocol is used unless explicitly opted in.
    bool     use_crc                 = false;
    uint32_t i2c_timeout_ms          = 10;
    // Full-scale pressure in bar for this specific sensor's calibrated
    // range -- CONFIRMED per DPRComputer::read_pressure() (reference
    // project): 100 bar for tank sensors, 400 bar for a COPV sensor. Only
    // this scale changes between part numbers; the raw-counts range and
    // temperature formula are the same for all of them.
    float    pressure_full_scale_bar = 100.0f;
};

struct Pte7300Measurement {
    int16_t pressure_raw;
    int16_t bridge_temperature_raw;
    int16_t status_raw;
    float   pressure_bar;    // computed from pressure_raw, see k_pte7300_* constants
    float   temperature_c;   // computed from bridge_temperature_raw
    uint8_t raw_bytes[7]; // pre-parse frame bytes, for protocol debugging
    uint8_t raw_len;      // valid bytes in raw_bytes (6, or 7 with CRC)
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
//
// EXPERIMENT (temporary): two different physical PCA9547PW-marked chips
// have both shown "digitally selected, switch never conducts" with VCC,
// GND, and the channel's local pull-up all confirmed healthy. Testing
// whether the populated part actually uses PCA9548A/TCA9548A-style
// bitmask semantics (one bit per channel, no separate enable bit) instead
// of genuine PCA9547 enable+select. If this is right, our old "channel 0"
// byte (0x08) was silently enabling channel 3 instead. Revert to the
// commented-out line below once this is confirmed either way.
static constexpr uint8_t pca9547_channel_control_byte(uint8_t channel)
{
    return static_cast<uint8_t>(1u << (channel & 0x07));
    // return static_cast<uint8_t>(k_pca9547_enable_bit | (channel & 0x07));
}

// ---------------------------------------------------------------------------
// PTE7300 address constants  (confirmed — Sensata PTE7300 datasheet)
// ---------------------------------------------------------------------------

static constexpr uint8_t k_pte7300_default_address_7bit    = 0x6C;
// 0xDA is listed as "address including CRC" — do NOT use directly with a HAL
// that expects a standard 7-bit address.
static constexpr uint8_t k_pte7300_address_with_crc_listed = 0xDA;

// ---------------------------------------------------------------------------
// PTE7300 register map  (CONFIRMED — ported from a working Teensy/Arduino
// driver (PTE7300_I2C.cpp, EPFL ERT FHI 2024-2025 DPR project) that read
// this exact sensor successfully. Not a trigger+fixed-frame protocol: the
// PTE7300 exposes memory-mapped 16-bit registers. A read is two separate
// I2C transactions -- write the 1-byte register address, then a fresh
// START to read N*2 bytes back. Each 16-bit word is little-endian on the
// wire (low byte first, high byte second) -- the opposite of what this
// driver originally assumed.
// ---------------------------------------------------------------------------

static constexpr uint8_t k_pte7300_reg_cmd    = 0x22; // command register (write-only)
static constexpr uint8_t k_pte7300_reg_serial = 0x50; // serial number, 2 words (32-bit)
static constexpr uint8_t k_pte7300_reg_dsp_t  = 0x2E; // temperature result
static constexpr uint8_t k_pte7300_reg_dsp_s  = 0x30; // pressure ("signal") result
static constexpr uint8_t k_pte7300_reg_status = 0x36; // status word
static constexpr uint8_t k_pte7300_reg_adc_tc = 0x26; // raw ADC / temp-compensation (unused here)

// 16-bit command codes, written little-endian (low byte first) to
// k_pte7300_reg_cmd as a single 3-byte transaction: [reg_addr, cmd_lo, cmd_hi].
static constexpr uint16_t k_pte7300_cmd_start = 0x8B93; // start a single measurement cycle
static constexpr uint16_t k_pte7300_cmd_sleep = 0x6C32;
static constexpr uint16_t k_pte7300_cmd_idle  = 0x7BBA;
static constexpr uint16_t k_pte7300_cmd_reset = 0xB169;

// Delay between issuing START and reading the result registers. The
// reference implementation didn't show this delay explicitly (it lived in
// the caller, which wasn't available to port), so this value is a safe
// margin, not a confirmed datasheet number -- tune down if a shorter delay
// still reads cleanly.
static constexpr uint32_t k_pte7300_start_delay_ms = 20;

// ---------------------------------------------------------------------------
// PTE7300 raw-to-engineering-units conversion  (CONFIRMED — ported from
// DPRComputer::read_pressure() / read_temperature(), same reference
// project). The raw-counts range is fixed regardless of part number; only
// the full-scale pressure differs (SensorConfig::pressure_full_scale_bar):
//   pressure_bar  = (pressure_raw - counts_min) * full_scale_bar
//                   / (counts_max - counts_min)
//   temperature_c = bridge_temperature_raw * temp_scale + temp_offset
// ---------------------------------------------------------------------------

static constexpr float k_pte7300_pressure_counts_min = -16000.0f;
static constexpr float k_pte7300_pressure_counts_max =  16000.0f;
static constexpr float k_pte7300_temp_scale           = 82.5f / 16000.0f;
static constexpr float k_pte7300_temp_offset          = 42.5f;

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

    // Which step the most recent read_measurement_raw()/read_device_serial()
    // call failed at (Pte7300Step::None if the last call succeeded).
    Pte7300Step last_step() const { return last_step_; }

private:
    Status ensure_mux_channel_selected();

    // Register read: writes the 1-byte register address, then reads
    // byte_len bytes back as a separate I2C transaction (matches the
    // reference beginTransmission/write/endTransmission + requestFrom
    // sequence -- two transactions, not a repeated start).
    Status read_register(uint8_t reg_addr, uint8_t* buf, size_t byte_len);

    // Command write: [k_pte7300_reg_cmd, cmd_lo, cmd_hi] in one transaction.
    Status write_command(uint16_t cmd);

    Pca9547Mux         mux_;
    SensorConfig       config_;
    I2C_HandleTypeDef* hi2c_;
    Pte7300Step        last_step_ = Pte7300Step::None;
};
