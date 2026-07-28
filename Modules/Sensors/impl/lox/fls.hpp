
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

using FLSModule = SensorModule<
    CommonTimerPolicy,
    SampleDoubleSensor,
    IfPipeline<
        average_pipeline::SimplePipeline<
            LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_FLS),
            FLS_WINDOW_SIZE,
            LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_FLS_mean)
        >
    >
>;
