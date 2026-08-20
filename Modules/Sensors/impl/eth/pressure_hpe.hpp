
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/eth/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

using PressureHpeSensorModule = PressureModule<
    CommonTimerPolicy,
    sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L4>>,

    // raw setter
    ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE),
    
    // mean setter
    PRESSURE_HPE_WINDOW_SIZE,
    ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_HPE_mean),

    sensata::SensataErrorPipeline<HPE_NAME>
>;
