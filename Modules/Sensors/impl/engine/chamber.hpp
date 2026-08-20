
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

using ChamberModule = BothModule<
    CommonTimerPolicy,
    sensata::BothSensata<sensata::SensataParams<SENSATA_CHANNEL_L2>>,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_C),
    PRESSURE_C_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_C_mean),
    
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_C),
    TEMPERATURE_C_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_C_mean),

    sensata::SensataErrorPipeline<CHAMBER_NAME>
>;
