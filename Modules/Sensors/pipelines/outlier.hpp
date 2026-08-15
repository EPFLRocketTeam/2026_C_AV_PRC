
#pragma once
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <sigutils/outlier.hpp>

namespace outlier_pipeline {

template<const size_t NumberInputs>
struct Frame {
    /* Array of values from the NumberInputs sensors */
    double values[NumberInputs];
    /* Whether the sensor is an outlier */
    bool is_outlier[NumberInputs];

    /* Number of used data points */
    size_t number_used;
    /* Final value computed */
    double value;
};

template<
    const size_t NumberKept,
    const size_t MinKept,
    const double &MinValue,
    const double &MaxValue
>
struct Params {
    static constexpr size_t number_kept = NumberKept;
    static constexpr size_t min_kept    = MinKept;

    static constexpr const double &min_value = MinValue;
    static constexpr const double &max_value = MaxValue;
};

template<
    typename    Params,
    typename    MeanPipeline,  // log the mean
    typename... RawPipelines   // log the raw values given to the pipeline
>
struct Pipeline {
private:
	static constexpr std::size_t InputCount = sizeof...(RawPipelines);

    using outlier_params = OutlierParams<double, InputCount, Params::number_kept>;

    MeanPipeline  meanPipeline;

    std::tuple<RawPipelines...> rawPipelines;
    
    outlier_params params;
public:
    Frame<InputCount> ingest (
            const std::array<double, InputCount> &values,
            const std::array<bool, InputCount> &valid) noexcept {
        Frame<InputCount> frame;
    
        for (size_t offset = 0; offset < InputCount; offset ++) {
            frame.values[offset] = values[offset];
            frame.is_outlier[offset] = !valid[offset];
        }

        params.min = Params::min_value;
        params.max = Params::max_value;

        frame.number_used = outlier<double, InputCount, Params::number_kept, false>(params, frame.values, frame.is_outlier, frame.value);

        std::apply([&frame, &valid](auto&... pipeline_instance){
            std::size_t idx = 0;
            ([&]{
                // printf("Sensor %d: valid=%d, outlier=%d, value=%f\n", (int) idx, (int) valid[idx], (int) frame.is_outlier[idx], (float) frame.values[idx]);
                if (valid[idx]) {
                    pipeline_instance.ingest(frame.values[idx]);
                }
                idx ++;
            }(), ...);
        }, rawPipelines);
        // printf("number_used=%d, value=%f\n", (int) frame.number_used, (float) frame.value);
        
        if (frame.number_used >= Params::min_kept) {
            meanPipeline.ingest(frame.value);
        }

        return frame;
    }
};

};
