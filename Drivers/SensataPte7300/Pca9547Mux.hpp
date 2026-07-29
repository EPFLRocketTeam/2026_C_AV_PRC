#pragma once

#include "stm32h7xx_hal.h"

#include <cstdint>

#include "./Types.hpp"  // Status, Result<T> -- adjust path if different

namespace sensata::mux {

// ---------------------------------------------------------------------------
// PCA9547 constants
// ---------------------------------------------------------------------------
//
// STATUS: channel-select byte semantics are UNRESOLVED, not confirmed.
//
// EXPERIMENT (ongoing): two different physical PCA9547PW-marked chips have
// both shown "digitally selected, switch never conducts" with VCC, GND, and
// the channel's local pull-up all confirmed healthy. Testing whether the
// populated part actually uses PCA9548A/TCA9548A-style bitmask semantics
// (one bit per channel, no separate enable bit) instead of genuine PCA9547
// enable+select. If so, the old "channel 0" byte (0x08, enable bit only)
// was silently enabling channel 3 instead.
//
// Current code path uses the bitmask hypothesis. Revert to the commented
// enable+select line once this is confirmed either way, and update this
// comment block and the class banner below when resolved.
inline constexpr uint8_t pca9547_channel_control_byte(uint8_t channel)
{
    return static_cast<uint8_t>(1u << (channel & 0x07));
    // return static_cast<uint8_t>(k_pca9547_enable_bit | (channel & 0x07));
}

inline constexpr uint8_t k_pca9547_default_address_7bit = 0x70; // A2=A1=A0=0
inline constexpr uint8_t k_pca9547_enable_bit           = 0x08; // control reg bit 3 (unused under current bitmask hypothesis)
inline constexpr uint8_t k_pca9547_disable_all          = 0x00;
inline constexpr uint8_t k_pca9547_max_channel          = 7;

// ===========================================================================
// Pca9547Mux
//
// Channel-select semantics: EXPERIMENTAL, see note above. Everything else
// (I2C framing, control-register read/write mechanics) is confirmed against
// NXP PCA9547 datasheet Rev.4.
// ===========================================================================
class Pca9547Mux {
public:
    Pca9547Mux(I2C_HandleTypeDef* hi2c, uint8_t address_7bit, uint32_t timeout_ms);

    /* Reading the control register is the canonical probe for PCA9547. */
    Status probe();

    // Selects a channel without a read-back verify, and without consulting
    // the cache. Prefer select_channel_cached() for normal use.
    Status select_channel(uint8_t channel);

    // Same as select_channel(), but reads the control register back
    // afterwards and confirms it matches the byte that was written, before
    // returning Ok. Use this when you need certainty that the mux actually
    // latched the requested channel, not just that the write was ACKed.
    // Does not consult or update the cache.
    Status select_channel_verified(uint8_t channel);

    // Skips the write+read-back entirely if the cache believes this
    // channel is already selected. Falls through to select_channel_verified()
    // otherwise, and updates the cache from the result.
    //
    // CACHING IS A TRUST ASSUMPTION, NOT A GUARANTEE: it can go stale if
    // another bus master touches this mux, if the mux resets independently
    // of this MCU (brown-out, ESD, shared reset glitch), or from bus-level
    // corruption that still produces an ACK. Mitigations: this cache is
    // invalidated on any I2C error and on any verify mismatch; callers
    // that need stronger guarantees should still periodically force a
    // select_channel_verified() call regardless of cache state.
    Status select_channel_cached(uint8_t channel);

    // Invalidates the cache without touching hardware. Call this after any
    // event that could have desynced the mux's real state from what this
    // object believes (detected bus recovery, external reset signal, etc).
    void invalidate_cache();

    Status disable_all_channels();

    Result<uint8_t> read_control_register();

private:
    Status write_control_byte(uint8_t byte);

    I2C_HandleTypeDef* hi2c_;
    uint8_t            address_7bit_;
    uint32_t           timeout_ms_;

    static constexpr uint8_t k_unknown_channel = 0xFF;
    uint8_t cached_channel_ = k_unknown_channel; // instance state -- NOT static, NOT shared across objects
};

} // namespace sensata::mux