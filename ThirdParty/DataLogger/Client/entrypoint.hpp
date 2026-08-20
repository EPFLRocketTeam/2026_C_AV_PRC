
#include <iostream>
#include "./container.hpp"

void run_decode (
    CsvChannelContainer &container,
    std::istream& stream
) {
    char buffer[1024];
    
    while (1) {
        LogHeader header;

        stream.read((char*) &header, sizeof(header));
        if (stream.fail()) {
            break ;
        }

        if (header.length > 1024) {
            throw std::runtime_error("Invalid header length (length = " + std::to_string(header.length) + " > 1024)");
        }

        stream.read(buffer, header.length);

        container.ingest(header, (const void*) buffer);
    }
}
