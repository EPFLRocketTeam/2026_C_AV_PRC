/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "../../Drivers/FDC1004/FDC1004_manual_test.h"
#include "../../Drivers/PT1000/main_test_pt1000.h"
#include "../../Drivers/SensataPte7300/SensataPte7300HardwareTest.hpp"
#include "../../Drivers/KULITE_CTL190/kulite_manual_test.hpp"
#include "../../Drivers/LMT85/lmt85_manual_test.hpp"
#include "../../Drivers/Valve/valve_manual_test.hpp"
#include "../../Drivers/Plume/Tests/Hardware/plume_manual_test.h"
#include "../../Application/FlightControl/prc_fsm_c_api.h"
#include "../../Application/FlightControl/prc_can.hpp"
#include "../../Application/main_app.h"
#include "../../Drivers/Plume/sd_hardware_init.h"
#include "CAN.h"
#include "usbd_cdc_if.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc3;

FDCAN_HandleTypeDef hfdcan1;

I2C_HandleTypeDef hi2c1;

SD_HandleTypeDef hsd1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_TIM4_Init(void);
static void MX_ADC3_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

int _write(int file, char *ptr, int len) {
    // Wait until USB is ready, but never wedge: if the CDC endpoint stays
    // busy (host not draining, missed completion), drop the output instead
    // of spinning forever.
	uint32_t start = HAL_GetTick();
    while (CDC_Transmit_HS((uint8_t*)ptr, len) == USBD_BUSY) {
        if (HAL_GetTick() - start > 100) {
            break;
        }
    }

#ifdef ENABLE_LOG
    // Relay the same bytes over CAN to FC, tagged with this board's role
    // (see Application/FlightControl/prc_can.cpp). Local VCP output above
    // is unaffected either way -- this is purely additive.
    Prc_Log_Forward(&hfdcan1, (const uint8_t*)ptr, (uint32_t)len);
#endif

    return len;
}

/* Plain I2C bus scanner, independent of any mux/channel selection -- just
 * sweeps every 7-bit address on the given bus and prints whatever ACKs.
 * Standard scan range 0x03-0x77 (0x00-0x07 and 0x78-0x7F are reserved).
 * Runs forever, one full sweep per iteration, 500 ms between sweeps. */
static void i2c_bus_scan(I2C_HandleTypeDef *hi2c)
{
    printf("=== I2C bus scan START (addr range 0x03-0x77) ===\r\n");
    while (1)
    {
        uint8_t foundCount = 0;
        for (uint8_t addr = 0x03; addr <= 0x77; addr++)
        {
            if (HAL_I2C_IsDeviceReady(hi2c, (uint16_t)(addr << 1), 3, 10) == HAL_OK)
            {
                printf("[I2C] found device at 0x%02X\r\n", addr);
                foundCount++;
            }
        }
        if (foundCount == 0)
        {
            printf("[I2C] no devices found\r\n");
        }
        else
        {
            printf("[I2C] scan done, %u device(s) found\r\n", foundCount);
        }
        HAL_Delay(500);
    }
}

/* STM32H743's internal temperature sensor: only available on ADC3 (already
 * used for PT1000), channel ADC_CHANNEL_TEMPSENSOR. Factory calibration
 * words TS_CAL1 (30 C)/TS_CAL2 (110 C), both @ VDDA=3.3V, live at fixed
 * addresses per the datasheet -- this CMSIS package doesn't define the
 * usual TEMPSENSOR_CAL1_ADDR/TEMPSENSOR_CAL2_ADDR macros, so they're
 * hardcoded here (matches UID_BASE 0x1FF1E800 + 0x20 / + 0x40). */
#define TEMPSENSOR_CAL1_ADDR ((uint16_t*)(0x1FF1E820UL))
#define TEMPSENSOR_CAL2_ADDR ((uint16_t*)(0x1FF1E840UL))
#define TEMPSENSOR_CAL1_TEMP (30)
#define TEMPSENSOR_CAL2_TEMP (110)

/* Reads and prints the MCU's internal die temperature via ADC3. Assumes
 * VDDA ~= 3.3V (the calibration reference voltage) -- if VDDA is known to
 * differ meaningfully on this board, scale raw16 by (VDDA_mV/3300) before
 * the calibration formula. */
static void print_mcu_temperature(void)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_810CYCLES_5; // temp sensor needs a long sample time
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset       = 0;
    sConfig.OffsetSignedSaturation = DISABLE;

    if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
    {
        printf("[TEMP] ConfigChannel failed\r\n");
        return;
    }

    if (HAL_ADC_Start(&hadc3) != HAL_OK)
    {
        printf("[TEMP] Start failed\r\n");
        return;
    }

    if (HAL_ADC_PollForConversion(&hadc3, 100) != HAL_OK)
    {
        HAL_ADC_Stop(&hadc3);
        printf("[TEMP] PollForConversion failed\r\n");
        return;
    }

    uint32_t raw16 = HAL_ADC_GetValue(&hadc3); // hadc3 is 16-bit resolution, matches TS_CAL width
    HAL_ADC_Stop(&hadc3);

    int32_t temp_c = (((int32_t)raw16 - (int32_t)(*TEMPSENSOR_CAL1_ADDR))
                       * (TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP))
                      / ((int32_t)(*TEMPSENSOR_CAL2_ADDR) - (int32_t)(*TEMPSENSOR_CAL1_ADDR))
                      + TEMPSENSOR_CAL1_TEMP;

    printf("[TEMP] MCU internal temperature: %ld C (raw=%lu)\r\n",
           (long)temp_c, (unsigned long)raw16);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);


  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  SCB_EnableICache();
  SCB_EnableDCache();

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_I2C1_Init();
  MX_SDMMC1_SD_Init();
  MX_TIM4_Init();
  MX_USB_DEVICE_Init();
  MX_ADC3_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  //manual_test_pt1000();  /* loops forever -- never returns, everything below this line won't run while active */

  //run_pte7300_i2c_scanner();
  //i2c_bus_scan(&hi2c1);
  //run_pte7300_channel0_scope_probe();
  //manual_test_ctl190();  /* loops forever -- never returns, everything below this line won't run while active */
  //manual_test_lmt85();  /* loops forever -- never returns, everything below this line won't run while active */
  //Valve_ManualTest();
  HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_4);

  main_init();

  /* Reset cause. Printed here (after Prc_Fsm_Init()'s buzzer delays), not
   * right after MX_USB_DEVICE_Init(), because USB CDC hasn't finished
   * enumerating with the host that early -- _write() silently drops
   * anything sent before the host's endpoint is ready. RCC->RSR itself
   * isn't touched by anything in between, so reading it late is safe;
   * RMVF is never written, so flags accumulate across resets until an
   * actual power-on reset clears them. */
  // printf("[RESET] RCC->RSR=0x%08lX%s%s%s%s%s%s%s%s%s\r\n",
  //        (unsigned long)RCC->RSR,
  //        (RCC->RSR & RCC_RSR_PORRSTF)   ? " POR"   : "",
  //        (RCC->RSR & RCC_RSR_PINRSTF)   ? " PIN"   : "",
  //        (RCC->RSR & RCC_RSR_BORRSTF)   ? " BOR"   : "",
  //        (RCC->RSR & RCC_RSR_SFTRSTF)   ? " SFT"   : "",
  //        (RCC->RSR & RCC_RSR_IWDG1RSTF) ? " IWDG1" : "",
  //        (RCC->RSR & RCC_RSR_WWDG1RSTF) ? " WWDG1" : "",
  //        (RCC->RSR & RCC_RSR_LPWRRSTF)  ? " LPWR"  : "",
  //        (RCC->RSR & RCC_RSR_D1RSTF)    ? " D1"    : "",
  //        (RCC->RSR & RCC_RSR_D2RSTF)    ? " D2"    : "");

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_Delay(1000);



  static uint32_t lastPeriodicTick = 0;
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  main_tick();
	  Prc_Fsm_Tick();

	  /*  Real CAN command decode, per Core/Inc/CAN.h's dictionary. Drains
	   *  the FIFO every iteration, unthrottled -- no HAL_Delay()/throttling
	   *  between reads, same reasoning as the LED slow-path split below (a
	   *  throttled decode would leave commands sitting in the FIFO for up
	   *  to 1s). See Application/FlightControl/prc_can.cpp for the actual
	   *  per-message decode/validation. */
	  if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0) > 0)
	  {
	    FDCAN_RxHeaderTypeDef rxHeader;
	    uint8_t rxData[8] = {0};
	    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
	    {
	      /* FDCAN_DLC_BYTES_0..8 (the only codes possible here -- classic
	       * frames, RxFifo0ElmtSize = FDCAN_DATA_BYTES_8) are numerically
	       * equal to the byte count, so DataLength can be used directly;
	       * no HAL conversion macro exists for it in this HAL version. */
	      Prc_Can_ProcessRxMessage(rxHeader.Identifier, rxData, rxHeader.DataLength);
	    }
	  }

	  // Slow path: LED blink + sensor print, throttled to a human-readable
	  // interval via HAL_GetTick() instead of a blocking HAL_Delay(), which
	  // would otherwise stall CAN RX (above) for the same duration.
	  if (HAL_GetTick() - lastPeriodicTick >= 1000)
	  {
	    lastPeriodicTick = HAL_GetTick();

	    // Blink each "_ON" indicator LED only while its matching setup jumper
	    // reads active. Assumed active-high (jumper pulls the input to 3V3) --
	    // if the LEDs never light, the jumpers are probably wired active-low:
	    // flip these to "== GPIO_PIN_RESET".
	    if (HAL_GPIO_ReadPin(ENG_SETUP_GPIO_Port, ENG_SETUP_Pin) == GPIO_PIN_SET) {
	      HAL_GPIO_TogglePin(ENG_ON_GPIO_Port, ENG_ON_Pin);
	    } else {
	      HAL_GPIO_WritePin(ENG_ON_GPIO_Port, ENG_ON_Pin, GPIO_PIN_RESET);
	    }
	    if (HAL_GPIO_ReadPin(LOX_SETUP_GPIO_Port, LOX_SETUP_Pin) == GPIO_PIN_SET) {
	      HAL_GPIO_TogglePin(LOX_ON_GPIO_Port, LOX_ON_Pin);
	    } else {
	      HAL_GPIO_WritePin(LOX_ON_GPIO_Port, LOX_ON_Pin, GPIO_PIN_RESET);
	    }
	    if (HAL_GPIO_ReadPin(ETH_SETUP_GPIO_Port, ETH_SETUP_Pin) == GPIO_PIN_SET) {
	      HAL_GPIO_TogglePin(ETH_ON_GPIO_Port, ETH_ON_Pin);
	    } else {
	      HAL_GPIO_WritePin(ETH_ON_GPIO_Port, ETH_ON_Pin, GPIO_PIN_RESET);
	    }

	    // Periodic DPR telemetry (tank + COPV pressures/temps), throttled to
	    // this same 1s tick. See Application/FlightControl/prc_can.cpp.
	    Prc_Can_SendTelemetry(&hfdcan1);

	    //run_pte7300_hardware_test();
	    //pte7300_print_data();
	    //FDC1004_ManualTest(&hi2c1);
	    //print_mcu_temperature();

	    //Valve_ManualTest();    /* TODO: verify Sol1-4/BV_CTRL wiring & NC/NO assumptions before running, see Drivers/Valve/Impl/ValveList.cpp */
	  }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 15;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInitStruct.PLL2.PLL2M = 1;
  PeriphClkInitStruct.PLL2.PLL2N = 10;
  PeriphClkInitStruct.PLL2.PLL2P = 2;
  PeriphClkInitStruct.PLL2.PLL2Q = 2;
  PeriphClkInitStruct.PLL2.PLL2R = 2;
  PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_3;
  PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOMEDIUM;
  PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
  PeriphClkInitStruct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_MultiModeTypeDef multimode = {0};
  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_16B;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.Oversampling.Ratio = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure the ADC multi-mode
  */
  multimode.Mode = ADC_MODE_INDEPENDENT;
  if (HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC3_Init(void)
{

  /* USER CODE BEGIN ADC3_Init 0 */

  /* USER CODE END ADC3_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC3_Init 1 */

  /* USER CODE END ADC3_Init 1 */

  /** Common config
  */
  hadc3.Instance = ADC3;
  hadc3.Init.Resolution = ADC_RESOLUTION_16B;
  hadc3.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc3.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc3.Init.LowPowerAutoWait = DISABLE;
  hadc3.Init.ContinuousConvMode = DISABLE;
  hadc3.Init.NbrOfConversion = 1;
  hadc3.Init.DiscontinuousConvMode = DISABLE;
  hadc3.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc3.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc3.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;
  hadc3.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc3.Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
  hadc3.Init.OversamplingMode = DISABLE;
  hadc3.Init.Oversampling.Ratio = 1;
  if (HAL_ADC_Init(&hadc3) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  sConfig.OffsetSignedSaturation = DISABLE;
  if (HAL_ADC_ConfigChannel(&hadc3, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC3_Init 2 */

  /* USER CODE END ADC3_Init 2 */

}

/**
  * @brief FDCAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_FDCAN1_Init(void)
{

  /* USER CODE BEGIN FDCAN1_Init 0 */

  /* USER CODE END FDCAN1_Init 0 */

  /* USER CODE BEGIN FDCAN1_Init 1 */

  /* USER CODE END FDCAN1_Init 1 */
  hfdcan1.Instance = FDCAN1;
  hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
  hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
  hfdcan1.Init.AutoRetransmission = ENABLE;
  hfdcan1.Init.TransmitPause = DISABLE;
  hfdcan1.Init.ProtocolException = DISABLE;
  hfdcan1.Init.NominalPrescaler = 1;
  hfdcan1.Init.NominalSyncJumpWidth = 1;
  hfdcan1.Init.NominalTimeSeg1 = 23;
  hfdcan1.Init.NominalTimeSeg2 = 8;
  hfdcan1.Init.DataPrescaler = 1;
  hfdcan1.Init.DataSyncJumpWidth = 1;
  hfdcan1.Init.DataTimeSeg1 = 1;
  hfdcan1.Init.DataTimeSeg2 = 1;
  hfdcan1.Init.MessageRAMOffset = 0;
  hfdcan1.Init.StdFiltersNbr = 2;
  hfdcan1.Init.ExtFiltersNbr = 0;
  hfdcan1.Init.RxFifo0ElmtsNbr = 4;
  hfdcan1.Init.RxFifo0ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxFifo1ElmtsNbr = 0;
  hfdcan1.Init.RxFifo1ElmtSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.RxBuffersNbr = 0;
  hfdcan1.Init.RxBufferSize = FDCAN_DATA_BYTES_8;
  hfdcan1.Init.TxEventsNbr = 0;
  hfdcan1.Init.TxBuffersNbr = 0;
  hfdcan1.Init.TxFifoQueueElmtsNbr = 32;
  hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
  hfdcan1.Init.TxElmtSize = FDCAN_DATA_BYTES_8;
  if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN FDCAN1_Init 2 */

  /* Accepts any message addressed to CAN_NODE_FC (node nibble only masked
   * -- priority/index free), per Core/Inc/CAN.h -- catches
   * CANID_BROADCAST_ABORT (0x000), CANID_TIME_SYNC (0x500), and any future
   * FC-broadcast message without needing a new filter added here. Index 1
   * (this board's own node) is configured later, once role detection has
   * run -- see Prc_Can_ConfigNodeFilter() in USER CODE 2. */
  FDCAN_FilterTypeDef canFilter;
  canFilter.IdType       = FDCAN_STANDARD_ID;
  canFilter.FilterIndex  = 0;
  canFilter.FilterType   = FDCAN_FILTER_MASK;
  canFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  canFilter.FilterID1    = (CAN_NODE_FC << 4);
  canFilter.FilterID2    = 0x0F0; /* mask: only the node nibble must match */
  if (HAL_FDCAN_ConfigFilter(&hfdcan1, &canFilter) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FDCAN_ConfigRxFifoOverwrite(&hfdcan1, FDCAN_RX_FIFO0, FDCAN_RX_FIFO_OVERWRITE) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE END FDCAN1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x307075B1;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SDMMC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SDMMC1_SD_Init(void)
{

  /* USER CODE BEGIN SDMMC1_Init 0 */
  sd_pre_init();
  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 4;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */
  sd_post_init(&hsd1);
  /* USER CODE END SDMMC1_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */

  uint32_t timclk = HAL_RCC_GetPCLK1Freq();
   // If APB1 prescaler != DIV1, timer clock is 2x pclk1 — check your clock config


   if (HAL_RCC_GetPCLK1Freq() != HAL_RCC_GetHCLKFreq()) timclk *= 2;
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = (timclk / 1000000U) - 1U; // → 1 MHz tick
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 19999; // 20 ms frame (50 Hz) for standard RC servo PWM
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, Buzzer_Pin|Sol1_ctrl_Pin|Sol2_ctrl_Pin|Sol3_ctrl_Pin
                          |Sol4_ctrl_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BOOT_LED_GPIO_Port, BOOT_LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, Igniter_Pin|BV_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, LEDCTRL_Pin|ETH_ON_Pin|LOX_ON_Pin|ENG_ON_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pins : Buzzer_Pin Sol1_ctrl_Pin Sol2_ctrl_Pin Sol3_ctrl_Pin
                           Sol4_ctrl_Pin */
  GPIO_InitStruct.Pin = Buzzer_Pin|Sol1_ctrl_Pin|Sol2_ctrl_Pin|Sol3_ctrl_Pin
                          |Sol4_ctrl_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : ENG_SETUP_Pin ETH_SETUP_Pin LOX_SETUP_Pin */
  GPIO_InitStruct.Pin = ENG_SETUP_Pin|ETH_SETUP_Pin|LOX_SETUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT_LED_Pin */
  GPIO_InitStruct.Pin = BOOT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOOT_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Igniter_Pin BV_CTRL_Pin */
  GPIO_InitStruct.Pin = Igniter_Pin|BV_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PWRGD_Pin */
  GPIO_InitStruct.Pin = PWRGD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PWRGD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDCTRL_Pin ETH_ON_Pin LOX_ON_Pin ENG_ON_Pin
                           PD6 */
  GPIO_InitStruct.Pin = LEDCTRL_Pin|ETH_ON_Pin|LOX_ON_Pin|ENG_ON_Pin
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : LIFTOFF_Pin */
  GPIO_InitStruct.Pin = LIFTOFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(LIFTOFF_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
