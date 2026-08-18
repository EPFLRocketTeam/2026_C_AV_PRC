
#include <meta>
#include <type_traits>

#include <iostream>
#include "./container.hpp"

int main (void) {
    CsvChannelContainer container(
        [](const std::string &_) -> std::ostream& {
            std::ostream &st = std::cout;
            return st;
        }
    );

    prc::DataDump dump;
    container.ingest({
        .magic = ETH_LOGGER_MAGIC,
        .record_type = engine::LOG_DATA_DUMP,
        .length = sizeof(dump),
        .timestamp_us = 124 
    }, &dump);
}
