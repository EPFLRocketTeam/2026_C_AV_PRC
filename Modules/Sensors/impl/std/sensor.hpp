
#pragma once
#include "Modules/Sensors/sensor.hpp"

struct pressure_temperature {
    double pressure    = 0;
    double temperature = 0;
};

struct SampleSensor {
    using sensor_result = result<pressure_temperature, no_error>;

    bool init () { return true; }
    sensor_result poll () {
        return sensor_result::success({ -1.0, -1.0 });
    }
};
struct SampleDoubleSensor {
    using sensor_result = result<double, no_error>;
    
    bool init () { return true; }
    sensor_result poll () {
        return sensor_result::success(-1.0);
    }
};

template<typename PressurePipeline, typename TemperaturePipeline>
struct SingleSensorPipeline {
private:
    PressurePipeline    pressure;
    TemperaturePipeline temperature;
public:
    void ingest (const pressure_temperature &data) {
        pressure   .ingest(data.pressure);
        temperature.ingest(data.temperature);
    }
};

template<
    typename PollPolicy,
    typename Sensor,
    typename RawPipeline, const size_t WindowSize, typename MeanPipeline,
    typename ErrorPipeline = NoPipeline
>
using PressureModule = SensorModule<
    PollPolicy,
    Sensor,
    IfPipeline<
        SingleSensorPipeline<
            average_pipeline::SimplePipeline<RawPipeline, WindowSize, MeanPipeline>,
            NoPipeline
        >,
        ErrorPipeline
    >
>;
template<
    typename PollPolicy,
    typename Sensor,
    typename RawPipeline, const size_t WindowSize, typename MeanPipeline,
    typename ErrorPipeline = NoPipeline
>
using TemperatureModule = SensorModule<
    PollPolicy,
    Sensor,
    IfPipeline<
        SingleSensorPipeline<
            NoPipeline,
            average_pipeline::SimplePipeline<RawPipeline, WindowSize, MeanPipeline>
        >,
        ErrorPipeline
    >
>;

template<typename PollPolicy, typename Sensor,
    typename PRawPipeline, const size_t PWindowSize, typename PMeanPipeline,
    typename TRawPipeline, const size_t TWindowSize, typename TMeanPipeline,
    typename ErrorPipeline = NoPipeline>
using BothModule = SensorModule<
    PollPolicy,
    Sensor,
    IfPipeline<
        SingleSensorPipeline<
            average_pipeline::SimplePipeline<PRawPipeline, PWindowSize, PMeanPipeline>,
            average_pipeline::SimplePipeline<TRawPipeline, TWindowSize, TMeanPipeline>
        >,
        ErrorPipeline
    >
>;

