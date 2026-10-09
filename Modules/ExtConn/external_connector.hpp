
#pragma once

#include <cstdint>
#include <cstddef>

#include "Application/app_printf.h"
#include "Application/app_timebase.h"

/*
 * External Connector Module
 *  - `bool ReadConn ()`: read connector value
 *  - `void SetValue (bool old, bool new)`: the value has changed.
 *  - `void PublishValue (bool old, bool new)`: the value has changed and needs to be published.
 *  - `PublishValueDeltaTimeMs`: minimal delay between two publish operations.
 */
template<
    auto ReadConn,
    auto SetValue,
    // publish at most once every 100 ms
    auto PublishValue,
    uint32_t PublishValueDeltaTimeMs = 100
>
struct ExternalConnectorModule {
private:
    bool had_first_measurement_ = false;
    bool last_value_ = false;
public:
    ExternalConnectorModule () = default;

    void tick () {
        // the module works with no continuity
        bool new_value = ReadConn();

        bool is_new_value = !had_first_measurement_ || (last_value_ != new_value);

        if (!had_first_measurement_) {
            last_value_ = new_value;
            had_first_measurement_ = true;
        }

        if (!is_new_value) {
            return ;
        }

        SetValue(last_value_, new_value);

        RUN_EVERY(PublishValueDeltaTimeMs) {
            PublishValue(last_value_, new_value);
        }
        
        last_value_ = new_value;
    }
};
