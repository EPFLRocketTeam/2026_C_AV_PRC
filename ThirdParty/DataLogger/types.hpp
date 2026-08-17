
#include <stddef.h>

struct LogHeader {
    uint8_t  magic;
    uint8_t  record_type;
    uint16_t length;
    uint32_t timestamp_us;
};

struct StorageHealth {
    uint32_t bytes_written_ = 0;
    uint32_t write_count_ = 0;
    uint32_t write_fail_count_ = 0;
    uint32_t max_write_time_us_ = 0;
    uint32_t tick_count_ = 0;
};
