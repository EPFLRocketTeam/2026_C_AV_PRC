
#pragma once
#include "Modules/Sensors/policy/timer.hpp"
#include "Modules/Sensors/policy/always.hpp"

#include "Modules/Sensors/pipelines/store.hpp"
#include "Modules/Sensors/pipelines/branch.hpp"
#include "Modules/Sensors/pipelines/nothing.hpp"
#include "Modules/Sensors/pipelines/average.hpp"
#include "Modules/Sensors/pipelines/outlier.hpp"

#include <cstdint>

template<
    typename PollPolicy,
    typename Sensor,
    typename DataPipeline
>
struct SensorModule {
private:
    PollPolicy   poll_policy;
    Sensor       sensor;
    DataPipeline pipeline;
public:
    Sensor& getSensor () { return sensor; }

    void tick () {
        if (!poll_policy.shouldPoll()) {
            return ;
        }

        poll_policy.registerPollStart();
        auto data = sensor.poll();
        poll_policy.registerPollEnd();

        pipeline.ingest(data);
    }
};
