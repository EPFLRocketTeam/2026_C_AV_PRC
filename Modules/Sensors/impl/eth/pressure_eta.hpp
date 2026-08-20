
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/eth/consts.hpp"
#include "Modules/Sensors/impl/std/multi.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

struct OnSuccessEtaPressure {
    void ingest (const auto &data) {
        ethLogger.logETAPressureFrame(*((eth::EtaPressureFrame*) &data));
    }
};

using PressureEtaSensorModule = multi::Module<
    CommonTimerPolicy,
    multi::PipelineParams<
        // Pressure
        multi::UseOutlier<
            outlier_pipeline::Params<
                PRESSURE_ETA_NUMBER_KEPT, 
                PRESSURE_ETA_MIN_NUMBER,
                PRESSURE_ETA_MIN_VALUE,
                PRESSURE_ETA_MAX_VALUE
            >,
            average_pipeline::MeanPipeline<
				PRESSURE_ETA_WINDOW_SIZE,
				ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA_mean)
			>
        >,
        // Temperature, simply unpack it. The module uses only pressure sensors
        //   so the temperature pipelines will be NoPipeline and after inlining
        //   that code will disappear
        multi::UseUnpack
    >,

    // Use only pressure sensors
    multi::PressureSensorParam<
        sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L3>>,
        ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA1),
        sensata::SensataErrorPipeline<ETA1_NAME, &ethLogger, &EthDataLogger<PlumeStorage>::logETA1PressureError>
    >,
    multi::PressureSensorParam<
        sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L2>>,
        ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA2),
        sensata::SensataErrorPipeline<ETA2_NAME, &ethLogger, &EthDataLogger<PlumeStorage>::logETA2PressureError>
    >,
    multi::PressureSensorParam<
        sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L4>>,
        ETH_SETTER_POLICY(prc::PropSensorsStoreEth::set_pressure_ETA3),
        sensata::SensataErrorPipeline<ETA3_NAME, &ethLogger, &EthDataLogger<PlumeStorage>::logETA3PressureError>
    >
>;
