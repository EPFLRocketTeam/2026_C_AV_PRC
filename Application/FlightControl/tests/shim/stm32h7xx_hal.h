// Host stand-in for the STM32 HAL, just enough for engine_state.cpp and
// the headers it pulls in to build under UNIT_TEST_ENV. The test provides
// HAL_GetTick and HAL_GPIO_WritePin and records every pin write.
#pragma once
#include <stdint.h>

typedef struct { uint32_t unused; } GPIO_TypeDef;
typedef struct { uint32_t unused; } TIM_HandleTypeDef;
typedef struct { uint32_t unused; } FDCAN_HandleTypeDef;
typedef struct { uint32_t unused; } SD_HandleTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;

extern GPIO_TypeDef test_gpiob;
#define GPIOB (&test_gpiob)
#define GPIO_PIN_2 ((uint16_t)0x0004)

uint32_t HAL_GetTick(void);
void HAL_GPIO_WritePin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state);
