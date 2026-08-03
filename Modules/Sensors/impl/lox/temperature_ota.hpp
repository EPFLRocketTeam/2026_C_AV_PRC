
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#include "Modules/Sensors/drivers/PT1000Sensor.hpp"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CREATE_TEMPERATURE_OTA_MODULE(Id, AdcChannel) \
    using TemperatureOtaSensorModule##Id = TemperatureModule< \
    CommonTimerPolicy, \
    pt1000::PT1000Sensor<pt1000::PT1000Params<AdcChannel, Id>>, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_temperature_OTA##Id), \
    TEMPERATURE_OTA_WINDOW_SIZE, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::CONCAT(CONCAT(set_temperature_OTA, Id), _mean)), \
    pt1000::PT1000ErrorPipeline<CONCAT(CONCAT(TOTA, Id), _NAME)> \
>;

CREATE_TEMPERATURE_OTA_MODULE(1, ADC_CHANNEL_10);
CREATE_TEMPERATURE_OTA_MODULE(2, ADC_CHANNEL_11);
CREATE_TEMPERATURE_OTA_MODULE(3, ADC_CHANNEL_0);
CREATE_TEMPERATURE_OTA_MODULE(4, ADC_CHANNEL_1);

#undef CONCAT_IMPL
#undef CONCAT
#undef CREATE_TEMPERATURE_OTA_MODULE
