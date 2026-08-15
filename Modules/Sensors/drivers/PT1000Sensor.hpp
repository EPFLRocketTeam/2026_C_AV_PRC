
#pragma once
#include "Drivers/PT1000/PT1000.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

extern "C" ADC_HandleTypeDef hadc3;

namespace pt1000 {
    inline constexpr float    DefaultRRefOhms = 1100.0f;
    inline constexpr uint32_t DefaultAdcMax   = 65535;

    template<
        uint32_t AdcChannel,

        const float&       RRefOhms = DefaultRRefOhms,
        uint32_t            AdcMax   = DefaultAdcMax,
        ADC_HandleTypeDef*  Hadc     = &hadc3
    >
    struct PT1000Params {
        static constexpr uint32_t adc_channel = AdcChannel;
        static constexpr float    r_ref_ohms  = RRefOhms;
        static constexpr uint32_t adc_max     = AdcMax;

        static constexpr ADC_HandleTypeDef* hadc = Hadc;
    };

    struct PT1000Error {
        uint32_t raw_adc;
    };

    template<typename Params>
    struct PT1000Sensor {
    private:
        using sensor_result = result<pressure_temperature, PT1000Error>;

        Drivers::PT1000::Config createConfig () {
            Drivers::PT1000::Config config;
            config.hadc        = Params::hadc;
            config.adc_channel = Params::adc_channel;
            config.r_ref       = Params::r_ref_ohms;
            config.adc_max     = Params::adc_max;

            return config;
        }

        Drivers::PT1000::PT1000Driver sensor;
    public:
        bool init () {
            sensor = Drivers::PT1000::PT1000Driver(createConfig());
            return sensor.init();
        }

        sensor_result poll () {
            Drivers::PT1000::PT1000Data data;
            if (!sensor.read(data)) {
                return sensor_result::error({ data.raw_adc });
            }

            pressure_temperature frame;
            frame.temperature = data.temperature;

            return sensor_result::success(frame);
        }
    };

    template<const char* (&SensorName)>
    struct PT1000ErrorPipeline {
        void ingest (const PT1000Error &error) {
            // Silenced -- unwired bench channels (e.g. ota2-4) spam this
            // every poll. Re-enable if you need to see PT1000 read
            // failures again.
            // printf("%s: read failed, raw_adc=%lu\r\n", SensorName, (unsigned long)error.raw_adc);
            (void)error;
        }
    };
};
