
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/PT1000Sensor.hpp"

template<const int SensorId>
class LoxOnTemperatureOTASuccess {
public:
	void ingest (const Drivers::PT1000::PT1000Data &data) {
		RUN_EVERY(1000)
			app_printf("[T-OTA-%d] adc=%u R=%f T=%f\r\n", SensorId, data.raw_adc, data.resistance, data.temperature);
	}
};

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CREATE_TEMPERATURE_OTA_MODULE(Id, AdcChannel) \
    using TemperatureOtaSensorModule##Id = TemperatureModule< \
    CommonTimerPolicy, \
    pt1000::PT1000Sensor<pt1000::PT1000Params<AdcChannel, OTA_RREF_OHMS##Id>, \
        LoxOnTemperatureOTASuccess<Id>>, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_temperature_OTA##Id), \
    TEMPERATURE_OTA_WINDOW_SIZE, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::CONCAT(CONCAT(set_temperature_OTA, Id), _mean)), \
    pt1000::PT1000ErrorPipeline<CONCAT(CONCAT(OTA, Id), _NAME)> \
>;

CREATE_TEMPERATURE_OTA_MODULE(1, PT1000_CHANNEL_T_OTA1);
CREATE_TEMPERATURE_OTA_MODULE(2, PT1000_CHANNEL_T_OTA2);
CREATE_TEMPERATURE_OTA_MODULE(3, PT1000_CHANNEL_T_OTA3);
CREATE_TEMPERATURE_OTA_MODULE(4, PT1000_CHANNEL_T_OTA4);

#undef CONCAT_IMPL
#undef CONCAT
#undef CREATE_TEMPERATURE_OTA_MODULE
