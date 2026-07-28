
#pragma once

#include "sigutils/outlier.hpp"
#include "Modules/Sensors/impl/common.hpp"
#include "Modules/Sensors/impl/lox/consts.hpp"
#include "Modules/Sensors/impl/std/sensor.hpp"

#define CONCAT_IMPL(a, b) a##b
#define CONCAT(a, b) CONCAT_IMPL(a, b)
#define CREATE_TEMPERATURE_OTA_MODULE(Id) \
    using TemperatureOtaSensorModule##Id = TemperatureModule< \
    CommonTimerPolicy, \
    SampleSensor, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::set_temperature_OTA##Id), \
    TEMPERATURE_OTA_WINDOW_SIZE, \
    LOX_SETTER_POLICY(prc::PropSensorsStoreLox::CONCAT(CONCAT(set_temperature_OTA, Id), _mean)) \
>;

CREATE_TEMPERATURE_OTA_MODULE(1);
CREATE_TEMPERATURE_OTA_MODULE(2);
CREATE_TEMPERATURE_OTA_MODULE(3);
CREATE_TEMPERATURE_OTA_MODULE(4);

#undef CONCAT_IMPL
#undef CONCAT
#undef CREATE_TEMPERATURE_OTA_MODULE
