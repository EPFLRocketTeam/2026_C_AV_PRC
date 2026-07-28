
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

using OxidizerInModule = BothModule<
    CommonTimerPolicy,
    SampleSensor,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_OIN),
    PRESSURE_OIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_OIN_mean),
    
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_OIN),
    TEMPERATURE_OIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_OIN_mean)
>;
