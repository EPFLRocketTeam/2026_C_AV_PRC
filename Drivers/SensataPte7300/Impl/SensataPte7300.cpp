#include "../SensataPte7300.hpp"

// STM32 HAL convention: DevAddress is the 7-bit address shifted left by 1.
// The R/W bit is appended internally by the peripheral.
static constexpr uint16_t to_hal_addr(uint8_t addr_7bit)
{
    return static_cast<uint16_t>(addr_7bit) << 1;
}

static Status hal_to_status(HAL_StatusTypeDef s)
{
    if (s == HAL_OK)      return Status::Ok;
    if (s == HAL_TIMEOUT) return Status::Timeout;
    return Status::I2cError;
}

// ===========================================================================
// Pca9547Mux  (fully confirmed — NXP PCA9547 datasheet Rev.4)
// ===========================================================================

Pca9547Mux::Pca9547Mux(I2C_HandleTypeDef* hi2c, uint8_t address_7bit, uint32_t timeout_ms)
    : hi2c_(hi2c), address_7bit_(address_7bit), timeout_ms_(timeout_ms)
{}

Status Pca9547Mux::probe()
{
    // Reading the control register is the canonical probe for PCA9547.
    return read_control_register().status;
}

Status Pca9547Mux::select_channel(uint8_t channel)
{
    if (channel > k_pca9547_max_channel) return Status::InvalidChannel;
    return write_control_byte(pca9547_channel_control_byte(channel));
}

Status Pca9547Mux::select_channel_verified(uint8_t channel)
{
    Status s = select_channel(channel);
    if (s != Status::Ok) return s;

    auto r = read_control_register();
    if (r.status != Status::Ok) return r.status;

    if (r.value != pca9547_channel_control_byte(channel)) {
        return Status::MuxChannelMismatch;
    }
    return Status::Ok;
}

Status Pca9547Mux::disable_all_channels()
{
    return write_control_byte(k_pca9547_disable_all);
}

Result<uint8_t> Pca9547Mux::read_control_register()
{
    uint8_t byte = 0;
    auto s = HAL_I2C_Master_Receive(
        hi2c_, to_hal_addr(address_7bit_), &byte, 1, timeout_ms_);
    if (s == HAL_OK) return {Status::Ok, byte};
    return {hal_to_status(s), 0};
}

Status Pca9547Mux::write_control_byte(uint8_t byte)
{
    return hal_to_status(HAL_I2C_Master_Transmit(
        hi2c_, to_hal_addr(address_7bit_), &byte, 1, timeout_ms_));
}

// ===========================================================================
// SensataPte7300
// ===========================================================================

SensataPte7300::SensataPte7300(I2C_HandleTypeDef* hi2c, SensorConfig config)
    : mux_(hi2c, config.mux_address_7bit, config.i2c_timeout_ms),
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
    Status s = mux_.probe();
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
    return mux_.select_channel(channel);
}

Status SensataPte7300::disable_mux_channels()
{
    return mux_.disable_all_channels();
}

Result<uint8_t> SensataPte7300::read_mux_control_register()
{
    return mux_.read_control_register();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

Status SensataPte7300::ensure_mux_channel_selected()
{
    // Verified select: confirms the mux actually latched the requested
    // channel (via readback) before any downstream I2C activity is
    // attempted, rather than just trusting that the write was ACKed.
    return mux_.select_channel_verified(config_.mux_channel);
}

// ASSUMED PROTOCOL — see header for debug guidance.
//
// Sequence:
//   1. Write trigger_cmd to sensor address (START a new on-demand cycle).
//   2. Wait k_pte7300_trigger_delay_ms (< 1 ms per datasheet; 1 ms = safe margin).
//   3. Read frame_len bytes from sensor address.
//
// If the sensor NACKs step 1, try removing the trigger write and going straight
// to step 3 — some sensors start a conversion on any I2C address match.
Status SensataPte7300::trigger_and_read(uint8_t trigger_cmd, uint8_t* buf, size_t frame_len)
{
    const uint16_t addr = to_hal_addr(config_.sensor_address_7bit);

    // Step 1: trigger
    auto s = HAL_I2C_Master_Transmit(
        hi2c_, addr, &trigger_cmd, 1, config_.i2c_timeout_ms);
    if (s != HAL_OK) return hal_to_status(s);

    // Step 2: wait for conversion
    HAL_Delay(k_pte7300_trigger_delay_ms);

    // Step 3: read
    s = HAL_I2C_Master_Receive(
        hi2c_, addr, buf, static_cast<uint16_t>(frame_len), config_.i2c_timeout_ms);
    return hal_to_status(s);
}

// CRC-8 — ASSUMED parameters (see header).
// Covers data[0 .. frame_len-2]; data[frame_len-1] is the received CRC byte.
bool SensataPte7300::verify_crc(const uint8_t* data, size_t frame_len)
{
    if (frame_len < 2) return false;

    uint8_t crc = k_crc8_init;
    for (size_t i = 0; i < frame_len - 1; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80u)
                ? static_cast<uint8_t>((crc << 1) ^ k_crc8_polynomial)
                : static_cast<uint8_t>(crc << 1);
        }
    }
    return crc == data[frame_len - 1];
}

// ---------------------------------------------------------------------------
// Sensor reads
//
// ASSUMED frame layout for read_measurement_raw:
//   buf[0] = pressure_H       buf[1] = pressure_L
//   buf[2] = bridge_temp_H    buf[3] = bridge_temp_L
//   buf[4] = status_H         buf[5] = status_L
//   buf[6] = CRC-8  (only when use_crc = true)
//
// All int16 values are big-endian, signed (direct 2's complement).
// 15-bit resolution: the MSB of each int16 is the sign bit.
//
// ASSUMED frame layout for read_device_serial:
//   buf[0..3] = serial (int32, big-endian)
//   buf[4]    = CRC-8  (only when use_crc = true)
// ---------------------------------------------------------------------------

Result<Pte7300Measurement> SensataPte7300::read_measurement_raw()
{
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, {}};

    const size_t frame_len = config_.use_crc
        ? k_measurement_with_crc
        : k_measurement_data_bytes;

    uint8_t buf[k_measurement_with_crc] = {};
    s = trigger_and_read(k_cmd_trigger_measurement, buf, frame_len);
    if (s != Status::Ok) return {s, {}};

    if (config_.use_crc && !verify_crc(buf, frame_len)) {
        return {Status::CrcError, {}};
    }

    Pte7300Measurement m;
    m.pressure_raw          = static_cast<int16_t>(
        (static_cast<uint16_t>(buf[0]) << 8) | buf[1]);
    m.bridge_temperature_raw = static_cast<int16_t>(
        (static_cast<uint16_t>(buf[2]) << 8) | buf[3]);
    m.status_raw            = static_cast<int16_t>(
        (static_cast<uint16_t>(buf[4]) << 8) | buf[5]);

    return {Status::Ok, m};
}

// Individual reads delegate to read_measurement_raw to avoid a second trigger.
Result<int16_t> SensataPte7300::read_pressure_raw()
{
    auto r = read_measurement_raw();
    return {r.status, r.value.pressure_raw};
}

Result<int16_t> SensataPte7300::read_bridge_temperature_raw()
{
    auto r = read_measurement_raw();
    return {r.status, r.value.bridge_temperature_raw};
}

Result<int16_t> SensataPte7300::read_status_raw()
{
    auto r = read_measurement_raw();
    return {r.status, r.value.status_raw};
}

Result<uint32_t> SensataPte7300::read_device_serial()
{
    Status s = ensure_mux_channel_selected();
    if (s != Status::Ok) return {s, 0};

    const size_t frame_len = config_.use_crc
        ? k_serial_with_crc
        : k_serial_data_bytes;

    uint8_t buf[k_serial_with_crc] = {};
    s = trigger_and_read(k_cmd_trigger_serial, buf, frame_len);
    if (s != Status::Ok) return {s, 0};

    if (config_.use_crc && !verify_crc(buf, frame_len)) {
        return {Status::CrcError, 0};
    }

    uint32_t serial =
        (static_cast<uint32_t>(buf[0]) << 24) |
        (static_cast<uint32_t>(buf[1]) << 16) |
        (static_cast<uint32_t>(buf[2]) <<  8) |
         static_cast<uint32_t>(buf[3]);

    return {Status::Ok, serial};
}
