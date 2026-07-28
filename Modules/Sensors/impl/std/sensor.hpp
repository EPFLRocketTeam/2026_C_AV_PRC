
#pragma once
#include "Modules/Sensors/sensor.hpp"
#include "random"

struct pressure_temperature {
    double pressure    = 0;
    double temperature = 0;
};



struct SampleSensor {
    using sensor_result = result<pressure_temperature, no_error>;

    bool init () { return true; }
    sensor_result poll () {
    	double r = rand() / (RAND_MAX + 1.0);   // [0, 1)
    	if (r < 0.5) {
    		return sensor_result::success({ .0, .0 });
    	} else {
    		return sensor_result::error({});
    	}

    }
};
struct SampleDoubleSensor {
    using sensor_result = result<double, no_error>;
    
    bool init () { return true; }

	sensor_result poll () {
		double r = rand() / (RAND_MAX + 1.0);   // [0, 1)
		if (r < 0.5) {
			return sensor_result::success({ .0 });
		} else {
			return sensor_result::error({});
		}

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

