
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/engine/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

using EthanolInModule = BothModule<
    CommonTimerPolicy,
    SampleSensor,

    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_EIN),
    PRESSURE_EIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_pressure_EIN_mean),
    
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_EIN),
    TEMPERATURE_EIN_WINDOW_SIZE,
    ENGINE_SETTER_POLICY(prc::PropSensorsStoreEngine::set_temperature_EIN_mean)
>;
