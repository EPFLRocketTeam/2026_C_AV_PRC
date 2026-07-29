
#include "Drivers/SensataPte7300/SensataPte7300.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

extern "C" I2C_HandleTypeDef hi2c1;

namespace sensata {
    static constexpr uint8_t  GLOBAL_MUX_ADDRESS     = 0x70; // A2=A1=A0=0
    static constexpr uint32_t GLOBAL_I2C_TIMEOUT_MS  = 10;
    static constexpr uint8_t  GLOBAL_I2C_SENSOR_ADDR = 0x6D; // 7-bit, excluding CRC

    inline constexpr float DefaultPressureFullScaleBar = 100.0f;

    extern mux::Pca9547Mux globalMux;
    
    template<
        uint8_t MuxChannel,
        uint8_t SensorAddress7Bit = GLOBAL_I2C_SENSOR_ADDR,

        bool     UseCRC = false,
        uint32_t I2CTimeout = GLOBAL_I2C_TIMEOUT_MS,
        const float& PressureFullScaleBar = DefaultPressureFullScaleBar
    >
    struct SensataParams {
        static constexpr uint8_t mux_channel = MuxChannel;
        static constexpr uint8_t sensor_address_7bit = SensorAddress7Bit;

        static constexpr bool use_crc = UseCRC;
        static constexpr uint32_t i2c_timeout = I2CTimeout;
        static constexpr float pressure_full_scale_bar = PressureFullScaleBar;
    };

    namespace internal {
        enum SensataPollMode {
            PRESSURE, TEMPERATURE
        };

        struct SensataError {
            Status      status;
            Pte7300Step step;

            SensataPollMode pollMode;
        };

        template<
            typename Params,
            const bool PollTemperature,
            const bool PollPressure,
            I2C_HandleTypeDef* hi2c = &hi2c1
        >
        struct SensataSensor {
        private:
            using sensor_result = result<pressure_temperature, SensataError>;

            sensata::SensorConfig createConfig () {
                sensata::SensorConfig config;
                config.mux_channel = Params::mux_channel;    // 0..7
                config.sensor_address_7bit = Params::sensor_address_7bit;
                config.use_crc = Params::use_crc;
                config.i2c_timeout_ms = Params::i2c_timeout;
                config.pressure_full_scale_bar = Params::pressure_full_scale_bar;

                return config;
            }

            sensata::SensataPte7300 sensor;
        public:
            bool init () {
                sensor = sensata::SensataPte7300(
                    &globalMux,
                    hi2c,
                    createConfig()
                );

                return true;
            }

            sensor_result poll () {
                pressure_temperature frame;

                if (PollPressure) {
                    auto result = sensor.read_pressure();
                    if (result.status != Status::Ok) {
                        SensataError error;
                        error.status = result.status;
                        error.step   = sensor.last_step();
                        error.pollMode = PRESSURE;

                        return sensor_result::error(error);
                    }

                    frame.pressure = result.value.pressure_bar;
                }

                if (PollTemperature) {
                    auto result = sensor.read_temperature();
                    if (result.status != Status::Ok) {
                        SensataError error;
                        error.status = result.status;
                        error.step   = sensor.last_step();
                        error.pollMode = TEMPERATURE;

                        return sensor_result::error(error);
                    }

                    frame.temperature = result.value.temperature_c;
                }
                
                return sensor_result::success(frame);
            }
        };

        const char* poll_mode_str (SensataPollMode mode);
    };



    template<const char* (&SensorName)>
    struct SensataErrorPipeline {
        void ingest (const internal::SensataError &error) {
            printf("%s: status=%s step=%s mode=%s\r\n", SensorName, status_str(error.status), step_str(error.step), internal::poll_mode_str(error.pollMode));
        }
    };


    template<typename Params, I2C_HandleTypeDef* hi2c = &hi2c1>
    using PressureSensata = internal::SensataSensor<Params, false, true, hi2c>;
    template<typename Params, I2C_HandleTypeDef* hi2c = &hi2c1>
    using TemperatureSensata = internal::SensataSensor<Params, true, false, hi2c>;
    template<typename Params, I2C_HandleTypeDef* hi2c = &hi2c1>
    using BothSensata = internal::SensataSensor<Params, true, true, hi2c>;
};
