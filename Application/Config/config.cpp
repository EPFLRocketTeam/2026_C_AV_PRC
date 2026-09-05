
#include "Application/Config/config.hpp"

namespace {
    FlightParamsManager    manager;
    FlightParamsCANChannel channel;
};

const FlightParams& config::get () {
    return manager.get_config();
}

uint32_t config::internal::crc_buffer () {
    return manager.get_buffer_crc();
}
uint32_t config::internal::crc_commited () {
    return manager.get_commited_crc();
}

void config::internal::commit () {
    manager.commit();
}
void config::internal::put (uint8_t offset, uint8_t* buffer, uint8_t buffer_length) {
    channel.load(manager.write_buffer(), offset, buffer, buffer_length);
}
