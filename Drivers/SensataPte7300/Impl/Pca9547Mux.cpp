#include "../Pca9547Mux.hpp"
#include <cstdio>
namespace sensata::mux {

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

} // namespace

Pca9547Mux::Pca9547Mux(I2C_HandleTypeDef* hi2c, uint8_t address_7bit, uint32_t timeout_ms)
    : hi2c_(hi2c), address_7bit_(address_7bit), timeout_ms_(timeout_ms)
{}

Status Pca9547Mux::probe()
{
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

Status Pca9547Mux::select_channel_cached(uint8_t channel)
{
    if (channel > k_pca9547_max_channel) return Status::InvalidChannel;

    if (cached_channel_ == channel) {
        return Status::Ok; // trust the cache -- no I2C traffic
    }

    Status s = select_channel_verified(channel);

    // Only trust the cache going forward if we actually confirmed the
    // channel via read-back. Any failure (I2C error OR mismatch) means we
    // no longer know the mux's true state, so fall back to "unknown"
    // rather than caching a channel we didn't verify.
    cached_channel_ = (s == Status::Ok) ? channel : k_unknown_channel;

    return s;
}

void Pca9547Mux::invalidate_cache()
{
    cached_channel_ = k_unknown_channel;
}

Status Pca9547Mux::disable_all_channels()
{
    Status s = write_control_byte(k_pca9547_disable_all);
    // Regardless of write success, the cache's belief "channel X is
    // selected" is no longer valid -- either the mux is now disabled, or
    // we don't actually know what state it's in.
    cached_channel_ = k_unknown_channel;
    return s;
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

} // namespace sensata::mux