
#pragma once

#include "Modules/Sensors/impl/std/sensor.hpp"
#include <cstdint>
#include <tuple>

namespace multi {

struct UseUnpack  {};
template<typename Params, typename MeanPipeline>
struct UseOutlier {
    using params        = Params;
    using mean_pipeline = MeanPipeline;
};


namespace internal {
    template<size_t NumberSensors>
    struct Frame {
        std::array<double, NumberSensors> pressures;
        std::array<double, NumberSensors> temperatures;
    };

    template<typename... Sensors>
    struct Sensor {
    private:
        std::tuple<Sensors...> sensors;

        static constexpr std::size_t NumberSensors = sizeof...(Sensors);
    public:
        Frame<NumberSensors> poll () {
            Frame<NumberSensors> frame;

            std::apply([&frame](auto&... pipeline_instance) {
                std::size_t idx = 0;
                ([&]{
                    pressure_temperature result = pipeline_instance.poll();
                    frame.pressures[idx] = result.pressure;
                    frame.temperatures[idx] = result.temperature;
                    idx ++;
                }(), ...);
            }, sensors);

            return frame;
        }
    };

    template<typename Use, size_t NumberInputs>
    struct PipelineReturnValueBuilder;
    template<size_t NumberInputs>
    struct PipelineReturnValueBuilder<UseUnpack, NumberInputs> {
        using type = std::array<double, NumberInputs>;
    };
    template<typename Params, typename MeanPipeline, size_t NumberInputs>
    struct PipelineReturnValueBuilder<UseOutlier<Params, MeanPipeline>, NumberInputs> {
        using type = outlier_pipeline::Frame<NumberInputs>;
    };

    template<typename UsePressure, typename UseTemperature, size_t NumberSensors>
    struct PipelineReturnValue {
        using pressure_type    = typename PipelineReturnValueBuilder<UsePressure,    NumberSensors>::type;
        using temperature_type = typename PipelineReturnValueBuilder<UseTemperature, NumberSensors>::type;
    
        pressure_type    pressure;
        temperature_type temperature;
    };

    template<typename Use, typename... RawPipelines>
    struct PipelineBuilder;

    template<typename... RawPipelines>
    struct PipelineBuilder<UseUnpack, RawPipelines...> {
        using type = BranchUnpackPipeline<RawPipelines...>;
    };
    template<typename Params, typename MeanPipeline, typename... RawPipelines>
    struct PipelineBuilder<UseOutlier<Params, MeanPipeline>, RawPipelines...> {
        using type = outlier_pipeline::Pipeline<Params, MeanPipeline, RawPipelines...>;
    };
};

template<typename Sensor, typename PPipeline, typename TPipeline> 
struct SensorParam {
    using sensor = Sensor;
    using pressure_pipeline = PPipeline;
    using temperature_pipeline = TPipeline;
};

template<typename Sensor, typename PPipeline>
using PressureSensorParam = SensorParam<Sensor, PPipeline, NoPipeline>;
template<typename Sensor, typename TPipeline>
using TemperatureSensorParam = SensorParam<Sensor, NoPipeline, TPipeline>;

template<typename UsePressure, typename UseTemperature, typename ReturnPipeline = NoPipeline>
struct PipelineParams {
    using use_pressure    = UsePressure;
    using use_temperature = UseTemperature;

    using return_pipeline = ReturnPipeline;
};

template<typename UsePressure, typename UseTemperature, typename ReturnPipeline, typename... SensorParams>
struct Pipeline {
private:
    static constexpr std::size_t NumberSensors = sizeof...(SensorParams);

    using return_type = internal::PipelineReturnValue<UsePressure, UseTemperature, NumberSensors>;

    using pressure_pipeline    = typename internal::PipelineBuilder<UsePressure, typename SensorParams::pressure_pipeline...>::type;
    using temperature_pipeline = typename internal::PipelineBuilder<UseTemperature, typename SensorParams::temperature_pipeline...>::type;

    pressure_pipeline    pressure;
    temperature_pipeline temperature;

    ReturnPipeline returnPipeline;
public:
    void ingest (const internal::Frame<NumberSensors> &data) {
        return_type ret;

        ret.pressure    = pressure.ingest(data.pressures);
        ret.temperature = temperature.ingest(data.temperatures);
    
        returnPipeline.ingest(ret);
    }
};

template<typename PollPolicy, typename Params, typename... SensorParams>
using Module = SensorModule<
    PollPolicy,
    internal::Sensor<typename SensorParams::sensor...>,
    Pipeline<
        typename Params::use_pressure, typename Params::use_temperature, typename Params::return_pipeline,
        SensorParams...
    >
>;

};
