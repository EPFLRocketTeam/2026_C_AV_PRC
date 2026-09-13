
#pragma once
#include "Drivers/LMT85/LMT85.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"
#include "Application/app_printf.h"

extern "C" ADC_HandleTypeDef hadc1;

namespace lmt85 {
    inline constexpr float    DefaultVrefMv = 3300.0f;
    inline constexpr uint32_t DefaultAdcMax = 65535;

    template<
        uint32_t AdcChannel = ADC_CHANNEL_5,

        uint32_t            AdcMax    = DefaultAdcMax,
        const float&        AdcVrefMv = DefaultVrefMv,
        ADC_HandleTypeDef*  Hadc      = &hadc1
    >
    struct LMT85Params {
        static constexpr uint32_t adc_channel = AdcChannel;
        static constexpr float    adc_vref_mv = AdcVrefMv;
        static constexpr uint32_t adc_max     = AdcMax;

        static constexpr ADC_HandleTypeDef* hadc = Hadc;
    };

    template<typename Params>
    struct LMT85Sensor {
    private:
        using sensor_result = result<Drivers::LMT85::LMT85Data, LMT85Error>;

        Drivers::LMT85::Config createConfig () {
            Drivers::LMT85::Config config;
            config.hadc        = Params::hadc;
            config.adc_channel = Params::adc_channel;
            config.adc_vref_mv = Params::adc_vref_mv;
            config.adc_max     = Params::adc_max;

            return config;
        }

        Drivers::LMT85::LMT85Driver driver;
    public:
        bool init () {
        	driver = Drivers::LMT85::LMT85Driver(createConfig());
            return driver.init();
        }

        sensor_result poll () {
            Drivers::LMT85::LMT85Data data;
            Drivers::LMT85::LMT85Status status = driver.read(data);
            if (status != Drivers::LMT85::LMT85Status::Ok) {
                return sensor_result::error({ status, data });
            }

            return sensor_result::success(data);
        }
    };

    inline const char* status_str (Drivers::LMT85::LMT85Status status) {
        switch (status) {
            case Drivers::LMT85::LMT85Status::Ok: return "Ok";
            case Drivers::LMT85::LMT85Status::HadcNullptr: return "HadcNullptr";
            case Drivers::LMT85::LMT85Status::ConfigFailed: return "ConfigFailed";
            case Drivers::LMT85::LMT85Status::StartFailed: return "StartFailed";
            case Drivers::LMT85::LMT85Status::PollFailed: return "PollFailed";
        }

        return "Unknown";
    }

    template<auto Func, auto Call>
    struct LMT85ErrorPipeline {
        void ingest (const LMT85Error &error) {
            RUN_EVERY(1000) {
                app_printf("[LMT85] FAIL status=%s\r\n",
                    status_str(error.status));
            }

            auto &logger = Func();
			(logger.*Call)(error);
        }
    };

    template<auto Func, auto Call>
    struct LMT85SuccessPipeline {
        void ingest (const Drivers::LMT85::LMT85Data &value) {
            prc::PrcStore::get_instance().set_prc_temperature(value.temperature);
            
            RUN_EVERY(1000) {
                app_printf("[LMT85] Board Temperature = %f\r\n",
                    value.temperature);
            }

            auto &logger = Func();
			(logger.*Call)(value);
        }
    };
};
