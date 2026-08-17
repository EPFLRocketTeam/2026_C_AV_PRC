
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

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
            average_pipeline::MeanPipeline<
				PRESSURE_OTA_WINDOW_SIZE,
				LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA_mean)
			>
        >,
        // Temperature, simply unpack it. The module uses only pressure sensors
        //   so the temperature pipelines will be NoPipeline and after inlining
        //   that code will disappear
        multi::UseUnpack
    >,

    // Use only pressure sensors
    multi::PressureSensorParam<
        sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L2>>,
        LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA1),
        sensata::SensataErrorPipeline<OTA1_NAME>
    >,
    // OTA2 physically not present on this board, commented out for now.
    // Also modify ThirdParty/DataLogger/types.hpp to change the pressure frame with
    //   number of sensors to 3 instead of 2.
    // Also modify ThirdParty/DataLogger/Client/rules.yaml to add the pressure frame
    //   data to the csv generator.
    // multi::PressureSensorParam<
    //     sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L2>>,
    //     LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA2),
    //     sensata::SensataErrorPipeline<OTA2_NAME>
    // >,
    multi::PressureSensorParam<
        sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L3>>,
        LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_OTA3),
        sensata::SensataErrorPipeline<OTA3_NAME>
    >
>;
