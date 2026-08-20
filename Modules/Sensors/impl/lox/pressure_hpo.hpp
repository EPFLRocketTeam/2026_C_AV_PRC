
#pragma once
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/SensataSensor.hpp"

struct OnSuccessHpo {
    void ingest (const auto &data) {
    	getLoxLogger().logHPO({
            .pressure      = prc::PrcStore::get_instance().propSensorsStoreLox.get_pressure_HPO(),
            .pressure_mean = prc::PrcStore::get_instance().propSensorsStoreLox.get_pressure_HPO_mean(),
        });
    }
};

using PressureHpoSensorModule = PressureModule<
    CommonTimerPolicy,
    sensata::PressureSensata<sensata::SensataParams<SENSATA_CHANNEL_L4>>,

    // raw setter
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO),
    
    // mean setter
    PRESSURE_HPO_WINDOW_SIZE,
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_pressure_HPO_mean),

    sensata::SensataErrorPipeline<HPO_NAME, &getLoxLogger, &LoxDataLogger<PlumeStorage>::logHPOError>,
    OnSuccessHpo
>;
