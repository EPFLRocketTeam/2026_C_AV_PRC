
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

using PressureHpoSensorModule = PressureModule<
    CommonTimerPolicy,
    sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L5>>,

    // raw setter
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO),
    
    // mean setter
    PRESSURE_HPO_WINDOW_SIZE,
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO_mean),

    sensata::SensataErrorPipeline<HPO_NAME>
>;
