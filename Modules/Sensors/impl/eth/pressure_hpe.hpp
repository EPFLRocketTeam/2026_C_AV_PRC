
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/eth/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

using PressureHpeSensorModule = PressureModule<
    CommonTimerPolicy,
    SampleSensor,

    // raw setter
    ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE),
    
    // mean setter
    PRESSURE_HPE_WINDOW_SIZE,
    ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE_mean)
>;
