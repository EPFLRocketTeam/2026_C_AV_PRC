#include "../SensataPte7300.hpp"

namespace sensata {

namespace {

// STM32 HAL convention: DevAddress is the 7-bit address shifted left by 1.
// The R/W bit is appended internally by the peripheral.
constexpr uint16_t to_hal_addr(uint8_t addr_7bit)
{
    return static_cast<uint16_t>(addr_7bit) << 1;
}

Status hal_to_status(HAL_StatusTypeDef s)
{
    if (s == HAL_OK)      return Status::Ok;
    if (s == HAL_TIMEOUT) return Status::Timeout;
    return Status::I2cError;
}

};

// ===========================================================================
// SensataPte7300
// ===========================================================================

SensataPte7300::SensataPte7300() {}
SensataPte7300::SensataPte7300(mux::Pca9547Mux* mux, I2C_HandleTypeDef* hi2c, SensorConfig config)
    : mux_(mux),
      config_(config),
      hi2c_(hi2c)
{}

Status SensataPte7300::init()
{
    Status s = probe_mux();
    if (s != Status::Ok) return s;
    return ensure_mux_channel_selected();
}

Status SensataPte7300::probe_mux()
{
    Status s = mux_->probe();
    if (s == Status::I2cError) return Status::MuxNotFound;
    return s;
}

Status SensataPte7300::probe_sensor()
{
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return s;

    auto hal_s = HAL_I2C_IsDeviceReady(
        hi2c_,
        to_hal_addr(config_.sensor_address_7bit),
        /* trials */ 3,
        config_.i2c_timeout_ms);

    if (hal_s == HAL_OK) return Status::Ok;
    if (hal_s == HAL_TIMEOUT) return Status::Timeout;
    return Status::SensorNotFound;
}

Status SensataPte7300::select_channel(uint8_t channel)
{
    return mux_->select_channel_cached(channel);
}

Status SensataPte7300::disable_mux_channels()
{
    return mux_->disable_all_channels();
}

Result<uint8_t> SensataPte7300::read_mux_control_register()
{
    return mux_->read_control_register();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

Status SensataPte7300::ensure_mux_channel_selected()
{
    // Verified select: confirms the mux actually latched the requested
    // channel (via readback) before any downstream I2C activity is
    // attempted, rather than just trusting that the write was ACKed.
    return mux_->select_channel_cached(config_.mux_channel);
}

// CONFIRMED register protocol — see header. Two separate I2C transactions:
// write the register address, then a fresh read of byte_len bytes.
Status SensataPte7300::read_register(uint8_t reg_addr, uint8_t* buf, size_t byte_len)
{
    const uint16_t addr = to_hal_addr(config_.sensor_address_7bit);

    auto s = HAL_I2C_Master_Transmit(hi2c_, addr, &reg_addr, 1, config_.i2c_timeout_ms);
    if (s != HAL_OK) return hal_to_status(s);

    s = HAL_I2C_Master_Receive(hi2c_, addr, buf, static_cast<uint16_t>(byte_len), config_.i2c_timeout_ms);
    return hal_to_status(s);
}

// [reg_addr, cmd_lo, cmd_hi] written as a single transaction.
Status SensataPte7300::write_command(uint16_t cmd)
{
    const uint16_t addr = to_hal_addr(config_.sensor_address_7bit);
    uint8_t frame[3] = {
        k_pte7300_reg_cmd,
        static_cast<uint8_t>(cmd & 0x00FFu),        // low byte first
        static_cast<uint8_t>((cmd >> 8) & 0x00FFu)  // high byte second
    };
    return hal_to_status(HAL_I2C_Master_Transmit(hi2c_, addr, frame, sizeof(frame), config_.i2c_timeout_ms));
}

// ---------------------------------------------------------------------------
// Sensor reads
//
// CONFIRMED sequence for read_measurement_raw (ported from the reference
// driver): write START to the CMD register, wait for the conversion, then
// read DSP_T (temperature), DSP_S (pressure) and STATUS as separate 16-bit
// little-endian register reads.
// ---------------------------------------------------------------------------

Result<pte7300_measurement> SensataPte7300::read_measurement_raw()
{
    last_step_ = Pte7300Step::MuxSelect;
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, {}};

    if (config_.use_crc) {
        // CRC-mode framing (address|1, CRC4 header + CRC8 payload) is not
        // yet ported -- see SensorConfig::use_crc.
        return {Status::ProtocolNotDefined, {}};
    }

    // EXPERIMENT: the CMD/START write NACKs even though a bare register
    // read (read_device_serial(), same mechanism) succeeds. Rather than
    // guess why, test the simplest explanation first: the sensor free-runs
    // continuously and result registers can just be read directly, exactly
    // like SERIAL already does -- no explicit trigger needed. If DSP_T/
    // DSP_S/STATUS read cleanly without ever writing START, this confirms
    // it and write_command()/k_pte7300_cmd_start can stay unused (or be
    // reserved for explicit sleep/idle/reset control instead).
    // last_step_ = Pte7300Step::WriteStart;
    // s = write_command(k_pte7300_cmd_start);
    // if (s != Status::Ok) return {s, {}};
    // HAL_Delay(k_pte7300_start_delay_ms);

    const auto temperature = read_temperature();
    if (temperature.status != Status::Ok) return {temperature.status, {}};
    const auto pressure = read_pressure();
    if (pressure.status != Status::Ok) return {pressure.status, {}};
    const auto status = read_status();
    if (status.status != Status::Ok) return {status.status, {}};

    pte7300_measurement frame;
    frame.temperature = temperature.value;
    frame.pressure = pressure.value;
    frame.status = status.value;

    return {Status::Ok, frame};
}

// Individual reads delegate to read_measurement_raw to avoid a second trigger.
Result<pte7300_pressure_frame> SensataPte7300::read_pressure()
{
    last_step_ = Pte7300Step::MuxSelect;
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, {}};

    pte7300_pressure_frame frame;

    last_step_ = Pte7300Step::ReadDspS;
    Status stat = read_register(k_pte7300_reg_dsp_s, frame.buf, sizeof(frame.buf));
    if (stat != Status::Ok) return {stat, {}};
    last_step_ = Pte7300Step::None;
    
    frame.pressure_raw = static_cast<int16_t>(
        (static_cast<uint16_t>(frame.buf[1]) << 8) | frame.buf[0]);
    frame.pressure_bar = (static_cast<float>(frame.pressure_raw) - k_pte7300_pressure_counts_min)
        * config_.pressure_full_scale_bar
        / (k_pte7300_pressure_counts_max - k_pte7300_pressure_counts_min);
    frame.pressure_psi = frame.pressure_bar * k_bar_to_psi;

    return {Status::Ok, frame};
}

Result<pte7300_temperature_frame> SensataPte7300::read_temperature()
{
    last_step_ = Pte7300Step::MuxSelect;
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, {}};
    
    pte7300_temperature_frame frame;

    last_step_ = Pte7300Step::ReadDspT;
    Status stat = read_register(k_pte7300_reg_dsp_t, frame.buf, sizeof(frame.buf));
    if (stat != Status::Ok) return {stat, {}};
    last_step_ = Pte7300Step::None;

    frame.bridge_temperature_raw = static_cast<int16_t>(
        (static_cast<uint16_t>(frame.buf[1]) << 8) | frame.buf[0]);
    frame.temperature_c = static_cast<float>(frame.bridge_temperature_raw) * k_pte7300_temp_scale
        + k_pte7300_temp_offset;

    return {Status::Ok, frame};
}

Result<pte7300_status_frame> SensataPte7300::read_status()
{
    last_step_ = Pte7300Step::MuxSelect;
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, {}};
    
    pte7300_status_frame frame;

    last_step_ = Pte7300Step::ReadDspS;
    Status stat = read_register(k_pte7300_reg_status, frame.buf, sizeof(frame.buf));
    if (stat != Status::Ok) return {stat, {}};
    last_step_ = Pte7300Step::None;
    
    frame.status_raw = static_cast<int16_t>(
        (static_cast<uint16_t>(frame.buf[1]) << 8) | frame.buf[0]);

    return {Status::Ok, frame};
}

// CONFIRMED: SERIAL register is 2 little-endian 16-bit words (4 bytes).
// serial = (word1 << 16) | word0.
Result<uint32_t> SensataPte7300::read_device_serial()
{
    last_step_ = Pte7300Step::MuxSelect;
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, 0};

    if (config_.use_crc) {
        return {Status::ProtocolNotDefined, 0};
    }

    last_step_ = Pte7300Step::ReadSerial;
    uint8_t buf[4] = {};
    s = read_register(k_pte7300_reg_serial, buf, sizeof(buf));
    if (s != Status::Ok) return {s, 0};

    last_step_ = Pte7300Step::None; // full success

    uint16_t word0 = static_cast<uint16_t>((static_cast<uint16_t>(buf[1]) << 8) | buf[0]);
    uint16_t word1 = static_cast<uint16_t>((static_cast<uint16_t>(buf[3]) << 8) | buf[2]);
    uint32_t serial = (static_cast<uint32_t>(word1) << 16) | static_cast<uint32_t>(word0);

    return {Status::Ok, serial};
}

}; // namespace sensata
