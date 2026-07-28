
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/eth/consts.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"

using PressureEtaSensorModule = multi::Module<
    CommonTimerPolicy,
    multi::PipelineParams<
        // Pressure
        multi::UseOutlier<
            outlier::Params<
                PRESSURE_ETA_NUMBER_KEPT, 
                PRESSURE_ETA_MIN_NUMBER,
                PRESSURE_ETA_MIN_VALUE,
                PRESSURE_ETA_MAX_VALUE
            >,
            ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA_mean)
        >,
        // Temperature, simply unpack it. The module uses only pressure sensors
        //   so the temperature pipelines will be NoPipeline and after inlining
        //   that code will disappear
        multi::UseUnpack
    >,

    // Use only pressure sensors
    multi::PressureSensorParam<SampleSensor, ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA1)>,
    multi::PressureSensorParam<SampleSensor, ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA2)>,
    multi::PressureSensorParam<SampleSensor, ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA3)>
>;
