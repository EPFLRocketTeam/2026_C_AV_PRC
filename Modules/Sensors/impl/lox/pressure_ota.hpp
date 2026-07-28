
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"

using PressureOtaSensorModule = multi::Module<
    CommonTimerPolicy,
    multi::PipelineParams<
        // Pressure
        multi::UseOutlier<
            outlier_pipeline::Params<
                PRESSURE_OTA_NUMBER_KEPT, 
                PRESSURE_OTA_MIN_NUMBER,
                PRESSURE_OTA_MIN_VALUE,
                PRESSURE_OTA_MAX_VALUE
            >,
            LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA_mean)
        >,
        // Temperature, simply unpack it. The module uses only pressure sensors
        //   so the temperature pipelines will be NoPipeline and after inlining
        //   that code will disappear
        multi::UseUnpack
    >,

    // Use only pressure sensors
    multi::PressureSensorParam<SampleSensor, LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA1)>,
    multi::PressureSensorParam<SampleSensor, LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA2)>,
    multi::PressureSensorParam<SampleSensor, LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA3)>
>;
