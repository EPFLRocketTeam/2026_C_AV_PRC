
#pragma once
#include "Modules/Sensors/sensor.hpp"

const size_t DEFAULT_NUMBER_TICKS = 10;

using CommonTimerPolicy = TimerPollPolicy<DEFAULT_NUMBER_TICKS>;
