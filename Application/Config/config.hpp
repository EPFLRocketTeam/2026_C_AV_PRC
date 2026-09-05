
#pragma once

#include "FlightParams.hpp"

#include <stdint.h>
#include <stddef.h>

namespace config {
    const FlightParams& get ();

    namespace internal {
        uint32_t crc_buffer ();
        uint32_t crc_commited ();

        void commit ();
        void put (uint8_t offset, uint8_t* buffer, uint8_t buffer_length);
    };
};
