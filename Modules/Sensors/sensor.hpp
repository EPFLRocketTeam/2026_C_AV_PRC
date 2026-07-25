
#pragma once
#include "Modules/Sensors/policy/poll/timer.hpp"
#include "Modules/Sensors/policy/poll/always.hpp"

#include "Modules/Sensors/policy/setter/store.hpp"
#include "Modules/Sensors/policy/setter/branch.hpp"

#include "Modules/Sensors/policy/filters/average.hpp"

#include <cstdint>

template<
    typename PollPolicy,
    typename Sensor,
    typename SetterPolicy
>
struct SensorModule {
private:
    PollPolicy   poll_policy;
    Sensor       sensor;
    SetterPolicy setter_policy;
public:
    Sensor& getSensor () { return sensor; }

    void tick () {
        if (!poll_policy.shouldPoll()) {
            return ;
        }

        poll_policy.registerPollStart();
        auto data = sensor.poll();
        poll_policy.registerPollEnd();

        setter_policy.set(data);
    }
};

struct SampleSensorModule {
    double poll () { return 0.; }
};
