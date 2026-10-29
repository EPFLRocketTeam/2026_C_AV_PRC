
#include "Application/Config/config.hpp"
#include "Application/app_printf.h"

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

// Same line as the FC's print_status: buffer crc => committed crc. The PRC
// holds a plain ConfigManager, so there is no per board table to add.
void config::internal::print_status () {
    app_printf("Config[PRC] : %lu => %lu\n",
               (unsigned long) manager.get_buffer_crc(),
               (unsigned long) manager.get_commited_crc());
}
