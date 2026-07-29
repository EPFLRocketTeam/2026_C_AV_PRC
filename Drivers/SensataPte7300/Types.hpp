
#pragma once
#include <cstdint>
#include <cstddef>

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
    float   pressure_psi;    // pressure_bar converted to psi
    float   temperature_c;   // computed from bridge_temperature_raw
    uint8_t raw_bytes[7]; // pre-parse frame bytes, for protocol debugging
    uint8_t raw_len;      // valid bytes in raw_bytes (6, or 7 with CRC)
};
