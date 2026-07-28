
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

using PressureHpoSensorModule = PressureModule<
    CommonTimerPolicy,
    SampleSensor,

    // raw setter
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO),
    
    // mean setter
    PRESSURE_HPO_WINDOW_SIZE,
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO_mean)
>;
