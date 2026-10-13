// Host stand-in for Core/Inc/main.h, igniter pin only (PB2, as on the board).
#pragma once
#include "stm32h7xx_hal.h"

#define Igniter_Pin GPIO_PIN_2
#define Igniter_GPIO_Port GPIOB
