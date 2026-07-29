
#pragma once
#include "./Types.hpp"
#include "./Pca9547Mux.hpp"

namespace sensata {

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

// 1 bar = 14.5037738 psi (exact, since 1 bar = 100000 Pa and 1 psi = 6894.757... Pa).
static constexpr float k_bar_to_psi = 14.5037738f;

// ---------------------------------------------------------------------------
// SensataPte7300
// ---------------------------------------------------------------------------

class SensataPte7300 {
public:
    SensataPte7300(mux::Pca9547Mux* mux, I2C_HandleTypeDef* hi2c, SensorConfig config);

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

    mux::Pca9547Mux*   mux_;
    SensorConfig       config_;
    I2C_HandleTypeDef* hi2c_;
    Pte7300Step        last_step_ = Pte7300Step::None;
};

}; // namespace sensata