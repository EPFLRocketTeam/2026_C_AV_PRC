/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Buzzer_Pin GPIO_PIN_6
#define Buzzer_GPIO_Port GPIOE
#define PT1000_1_Pin GPIO_PIN_0
#define PT1000_1_GPIO_Port GPIOC
#define PT1000_2_Pin GPIO_PIN_1
#define PT1000_2_GPIO_Port GPIOC
#define PT1000_3_Pin GPIO_PIN_2
#define PT1000_3_GPIO_Port GPIOC
#define PT1000_4_Pin GPIO_PIN_3
#define PT1000_4_GPIO_Port GPIOC
#define ENG_SETUP_Pin GPIO_PIN_1
#define ENG_SETUP_GPIO_Port GPIOA
#define LOX_SETUP_Pin GPIO_PIN_2
#define LOX_SETUP_GPIO_Port GPIOA
#define ETH_SETUP_Pin GPIO_PIN_3
#define ETH_SETUP_GPIO_Port GPIOA
#define Kulite_Pin GPIO_PIN_4
#define Kulite_GPIO_Port GPIOA
#define BOOT_LED_Pin GPIO_PIN_4
#define BOOT_LED_GPIO_Port GPIOC
#define Vtemp_Pin GPIO_PIN_1
#define Vtemp_GPIO_Port GPIOB
#define Igniter_Pin GPIO_PIN_2
#define Igniter_GPIO_Port GPIOB
#define Sol1_ctrl_Pin GPIO_PIN_7
#define Sol1_ctrl_GPIO_Port GPIOE
#define Sol2_ctrl_Pin GPIO_PIN_8
#define Sol2_ctrl_GPIO_Port GPIOE
#define Sol3_ctrl_Pin GPIO_PIN_9
#define Sol3_ctrl_GPIO_Port GPIOE
#define Sol4_ctrl_Pin GPIO_PIN_10
#define Sol4_ctrl_GPIO_Port GPIOE
#define PWRGD_Pin GPIO_PIN_10
#define PWRGD_GPIO_Port GPIOD
#define LEDCTRL_Pin GPIO_PIN_15
#define LEDCTRL_GPIO_Port GPIOD
#define ETH_ON_Pin GPIO_PIN_3
#define ETH_ON_GPIO_Port GPIOD
#define LOX_ON_Pin GPIO_PIN_4
#define LOX_ON_GPIO_Port GPIOD
#define ENGONE_ON_Pin GPIO_PIN_5
#define ENGONE_ON_GPIO_Port GPIOD
#define BV_CTRL_Pin GPIO_PIN_5
#define BV_CTRL_GPIO_Port GPIOB
#define PWM_BV_Pin GPIO_PIN_8
#define PWM_BV_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
