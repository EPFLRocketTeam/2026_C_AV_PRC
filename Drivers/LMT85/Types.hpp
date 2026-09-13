#pragma once
#include <cstddef>
#include <cstdint>

namespace Drivers::LMT85 {
    // ======================================================================
    // DATA STRUCTURES
    // ======================================================================

    struct LMT85Data {
        float    voltage_mv;  // Measured output voltage, in mV
        float    temperature; // Temperature in degrees Celsius
        uint32_t raw_adc;     // Raw ADC reading
        bool     valid;       // true if the reading is valid
    };
    static_assert(sizeof(LMT85Data) == 16);

    enum LMT85Status : uint8_t {
        Ok,
        HadcNullptr,
        ConfigFailed,
        StartFailed,
        PollFailed
    };
    static_assert(sizeof(LMT85Status) == 1);
};

namespace lmt85 {  
    struct LMT85Error {
        Drivers::LMT85::LMT85Status status;
        Drivers::LMT85::LMT85Data data;
    };
    static_assert(sizeof(LMT85Error) == 20);
};
