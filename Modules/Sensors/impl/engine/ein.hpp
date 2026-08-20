
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"
#include "Modules/Sensors/drivers/PT1000Sensor.hpp"

struct OnSuccessPressureEin {
    void ingest (const auto &data) {
        engineLogger.logEinPFrame({
            .pressure      = prc::PrcStore::get_instance().propSensorsStoreEngine.get_pressure_EIN(),
            .pressure_mean = prc::PrcStore::get_instance().propSensorsStoreEngine.get_pressure_EIN_mean(),
        });
    }
};

using EthanolInModule = PressureModule<
    CommonTimerPolicy,
    sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L3>>,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_EIN),
    PRESSURE_EIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_EIN_mean),

    sensata::SensataErrorPipeline<EIN_NAME, &engineLogger, &EngineDataLogger<PlumeStorage>::logEinPError>,
    OnSuccessPressureEin
>;

using TemperatureEinModule = TemperatureModule<
    CommonTimerPolicy,
    pt1000::PT1000Sensor<pt1000::PT1000Params<PT1000_CHANNEL_T_EIN>>,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_EIN),
    TEMPERATURE_EIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_EIN_mean),

    pt1000::PT1000ErrorPipeline<TEIN_NAME>
>;
