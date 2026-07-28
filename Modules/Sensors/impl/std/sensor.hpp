
#pragma once
#include "Modules/Sensors/sensor.hpp"

struct pressure_temperature {
    double pressure    = 0;
    double temperature = 0;
};

struct SampleSensor {
    pressure_temperature poll () { return { 0., 0. }; }
};
struct SampleDoubleSensor {
    double poll () { return 0.; }
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

template<typename PollPolicy, typename Sensor, typename RawPipeline, const size_t WindowSize, typename MeanPipeline>
using PressureModule = SensorModule<
    PollPolicy,
    Sensor,
    SingleSensorPipeline<
        average_pipeline::SimplePipeline<RawPipeline, WindowSize, MeanPipeline>,
        NoPipeline
    >
>;
template<typename PollPolicy, typename Sensor, typename RawPipeline, const size_t WindowSize, typename MeanPipeline>
using TemperatureModule = SensorModule<
    PollPolicy,
    Sensor,
    SingleSensorPipeline<
        NoPipeline,
        average_pipeline::SimplePipeline<RawPipeline, WindowSize, MeanPipeline>
    >
>;

template<typename PollPolicy, typename Sensor,
    typename PRawPipeline, const size_t PWindowSize, typename PMeanPipeline,
    typename TRawPipeline, const size_t TWindowSize, typename TMeanPipeline>
using BothModule = SensorModule<
    PollPolicy,
    Sensor,
    SingleSensorPipeline<
        average_pipeline::SimplePipeline<PRawPipeline, PWindowSize, PMeanPipeline>,
        average_pipeline::SimplePipeline<TRawPipeline, TWindowSize, TMeanPipeline>
    >
>;
    
