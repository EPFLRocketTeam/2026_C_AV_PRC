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
#include "../../Drivers/Valve/valve_manual_test.hpp"
#include "usbd_cdc_if.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/*  CAN bus test between 2026_C_AV_PRC and 2026_C_AV_FC (PD0=RX, PD1=TX on both boards).  */
#define CANBUS_TEST_TX_ID   0x100u   /*  PRC -> FC  */
#define CANBUS_TEST_RX_ID   0x101u   /*  FC -> PRC  */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc3;

FDCAN_HandleTypeDef hfdcan1;

I2C_HandleTypeDef hi2c1;

SD_HandleTypeDef hsd1;

TIM_HandleTypeDef htim4;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_FDCAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_SDMMC1_SD_Init(void);
static void MX_TIM4_Init(void);
static void MX_ADC3_Init(void);
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
    return len;
}

/* Drains FDCAN1 RX_FIFO0 and prints every received message: ID, DLC, and
 * the full data payload (not just the first byte). Call every loop
 * iteration -- draining fully each time avoids reading stale entries when
 * traffic arrives faster than the loop period. */
static void print_fdcan_rx_messages(void)
{
    uint32_t rxFillLevel;
    printf("Trying RX\r\n");
    while ((rxFillLevel = HAL_FDCAN_GetRxFifoFillLevel(&hfdcan1, FDCAN_RX_FIFO0)) > 0)
    {
        FDCAN_RxHeaderTypeDef rxHeader;
        uint8_t rxData[8] = {0};
        printf("In loop \r\n");
        if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
        {
            printf("[CAN] RX id=0x%03lX dlc=%lu ts=%lu data=",
                   (unsigned long)rxHeader.Identifier,
                   (unsigned long)rxHeader.DataLength,
                   (unsigned long)rxHeader.RxTimestamp);
            for (uint32_t i = 0; i < rxHeader.DataLength; i++)
            {
                printf("%02X ", rxData[i]);
            }
            printf("(fill=%lu)\r\n", (unsigned long)rxFillLevel);
        }
        else
        {
            printf("[CAN] RX read failed\r\n");
        }
    }
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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_FDCAN1_Init();
  MX_I2C1_Init();
  //MX_SDMMC1_SD_Init();
  MX_TIM4_Init();
  MX_USB_DEVICE_Init();
  MX_ADC3_Init();
  /* USER CODE BEGIN 2 */


  //manual_test_pt1000();

  //run_pte7300_i2c_scanner();
  //i2c_bus_scan(&hi2c1);
  //run_pte7300_channel0_scope_probe();
  //manual_test_ctl190();  /* one-shot ~32s test, then returns */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  HAL_Delay(1000);

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_3);
	  HAL_Delay(100);
	  //run_pte7300_hardware_test();
	  FDC1004_ManualTest(&hi2c1);

	  //Valve_ManualTest();    /* TODO: verify Sol1-4/BV_CTRL wiring & NC/NO assumptions before running, see Drivers/Valve/Impl/ValveList.cpp */

	  /*{

	    static uint8_t canTestCounter = 0x01;  // nonzero start: makes stale/zeroed reads obvious

	    FDCAN_TxHeaderTypeDef txHeader;
	    txHeader.Identifier          = CANBUS_TEST_TX_ID;
	    txHeader.IdType              = FDCAN_STANDARD_ID;
	    txHeader.TxFrameType         = FDCAN_DATA_FRAME;
	    txHeader.DataLength          = FDCAN_DLC_BYTES_1;
	    txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
	    txHeader.BitRateSwitch       = FDCAN_BRS_OFF;
	    txHeader.FDFormat            = FDCAN_CLASSIC_CAN;
	    txHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
	    txHeader.MessageMarker       = 0;
*/

	    /*  HAL_FDCAN_AddMessageToTxFifoQ always word-copies 4 bytes regardless
	     *  of DataLength, so the buffer must be padded to a 4-byte boundary
	     *  even though only 1 byte is actually put on the wire (per DLC). */
	  /*
	    uint8_t txData[4] = { canTestCounter, 0, 0, 0 };
	    if (HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &txHeader, txData) == HAL_OK)
	    {
	      printf("[CAN] TX id=0x%03lX data=0x%02X\r\n", (unsigned long)CANBUS_TEST_TX_ID, canTestCounter);
	      canTestCounter++;
	    }
	    else
	    {*/
	      /* Decode ErrorCode so we know exactly which of the 3 possible
	       * HAL_FDCAN_AddMessageToTxFifoQ failure paths this is:
	       *   NOT_STARTED (0x08) -> hfdcan1.State != HAL_FDCAN_STATE_BUSY
	       *   PARAM       (0x20) -> TXBC.TFQS == 0 (Tx FIFO/Queue never
	       *                          got allocated in message RAM)
	       *   FIFO_FULL   (0x200)-> queue genuinely full (32 pending,
	       *                          nothing ever actually transmitted) */
	      /*printf("[CAN] TX failed, ErrorCode=0x%08lX state=%d\r\n",
	             (unsigned long)hfdcan1.ErrorCode, (int)hfdcan1.State);
	    }*/

	    /*  Drain the whole FIFO each iteration -- popping only one message
	     *  per loop let the queue back up (self-reception + peer traffic
	     *  arriving faster than we drained it), so we kept reading stale
	     *  entries instead of the newest one. */
	    /*print_fdcan_rx_messages();

	    {
	      FDCAN_ProtocolStatusTypeDef protoStatus;
	      FDCAN_ErrorCountersTypeDef  errCounters;
	      HAL_FDCAN_GetProtocolStatus(&hfdcan1, &protoStatus);
	      HAL_FDCAN_GetErrorCounters(&hfdcan1, &errCounters);
	      printf("[CAN] status busoff=%lu errpass=%lu warn=%lu lastErr=%lu tec=%lu rec=%lu\r\n",
	             protoStatus.BusOff, protoStatus.ErrorPassive, protoStatus.Warning,
	             protoStatus.LastErrorCode, errCounters.TxErrorCnt, errCounters.RxErrorCnt);
	      printf("[CAN] msgRam StdFilterSA=0x%08lX RxFIFO0SA=0x%08lX TxFIFOQSA=0x%08lX\r\n",
	             (unsigned long)hfdcan1.msgRam.StandardFilterSA,
	             (unsigned long)hfdcan1.msgRam.RxFIFO0SA,
	             (unsigned long)hfdcan1.msgRam.TxFIFOQSA);
	    }
	  }*/



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
  hfdcan1.Init.AutoRetransmission = DISABLE;
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
  hfdcan1.Init.StdFiltersNbr = 1;
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

  FDCAN_FilterTypeDef canFilter;
  canFilter.IdType       = FDCAN_STANDARD_ID;
  canFilter.FilterIndex  = 0;
  canFilter.FilterType   = FDCAN_FILTER_MASK;
  canFilter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  canFilter.FilterID1    = CANBUS_TEST_RX_ID;
  canFilter.FilterID2    = 0x7FF;  /* exact-match mask: only CANBUS_TEST_RX_ID accepted */
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

  /* USER CODE END SDMMC1_Init 0 */

  /* USER CODE BEGIN SDMMC1_Init 1 */

  /* USER CODE END SDMMC1_Init 1 */
  hsd1.Instance = SDMMC1;
  hsd1.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
  hsd1.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
  hsd1.Init.BusWide = SDMMC_BUS_WIDE_4B;
  hsd1.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd1.Init.ClockDiv = 0;
  if (HAL_SD_Init(&hsd1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SDMMC1_Init 2 */

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
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
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
  HAL_GPIO_WritePin(GPIOD, LEDCTRL_Pin|ETH_ON_Pin|LOX_ON_Pin|ENGONE_ON_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_6, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BV_CTRL_GPIO_Port, BV_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Buzzer_Pin Sol1_ctrl_Pin Sol2_ctrl_Pin Sol3_ctrl_Pin
                           Sol4_ctrl_Pin */
  GPIO_InitStruct.Pin = Buzzer_Pin|Sol1_ctrl_Pin|Sol2_ctrl_Pin|Sol3_ctrl_Pin
                          |Sol4_ctrl_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : ENG_SETUP_Pin LOX_SETUP_Pin ETH_SETUP_Pin */
  GPIO_InitStruct.Pin = ENG_SETUP_Pin|LOX_SETUP_Pin|ETH_SETUP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : Kulite_Pin */
  GPIO_InitStruct.Pin = Kulite_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Kulite_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT_LED_Pin */
  GPIO_InitStruct.Pin = BOOT_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOOT_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Vtemp_Pin */
  GPIO_InitStruct.Pin = Vtemp_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Vtemp_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : Igniter_Pin */
  GPIO_InitStruct.Pin = Igniter_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(Igniter_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PWRGD_Pin */
  GPIO_InitStruct.Pin = PWRGD_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(PWRGD_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LEDCTRL_Pin ETH_ON_Pin LOX_ON_Pin ENGONE_ON_Pin
                           PD6 */
  GPIO_InitStruct.Pin = LEDCTRL_Pin|ETH_ON_Pin|LOX_ON_Pin|ENGONE_ON_Pin
                          |GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : BV_CTRL_Pin */
  GPIO_InitStruct.Pin = BV_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BV_CTRL_GPIO_Port, &GPIO_InitStruct);

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
