
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"
#include "Modules/Sensors/drivers/PT1000Sensor.hpp"

struct OnSuccessPressureOin {
    void ingest (const auto &data) {
        engineLogger.logOinPFrame({
            .pressure      = prc::PrcStore::get_instance().propSensorsStoreEngine.get_pressure_OIN(),
            .pressure_mean = prc::PrcStore::get_instance().propSensorsStoreEngine.get_pressure_OIN_mean(),
        });
    }
};

using OxidizerInModule = PressureModule<
    CommonTimerPolicy,
    sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L4>>,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_OIN),
    PRESSURE_OIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_OIN_mean),

    sensata::SensataErrorPipeline<OIN_NAME, &engineLogger, &EngineDataLogger<PlumeStorage>::logOinPError>,
    OnSuccessPressureOin
>;

using TemperatureOinModule = TemperatureModule<
    CommonTimerPolicy,
    pt1000::PT1000Sensor<pt1000::PT1000Params<PT1000_CHANNEL_T_OIN>>,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_OIN),
    TEMPERATURE_OIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_OIN_mean),

    pt1000::PT1000ErrorPipeline<TOIN_NAME>
>;
