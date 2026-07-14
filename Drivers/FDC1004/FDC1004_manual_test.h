//    FILE: FDC1004_manual_test.h
// PURPOSE: C-callable smoke test for the FDC1004 driver, for use from main.c.

#ifndef FDC1004_MANUAL_TEST_H
#define FDC1004_MANUAL_TEST_H

#include "stm32h7xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

extern volatile int   fdc_connected;
extern volatile float fdc_cap_ch0_pf;
extern volatile float fdc_cap_ch1_pf;
extern volatile float fdc_cap_ch2_pf;
extern volatile float fdc_cap_ch3_pf;
extern volatile int   fdc_last_error;

void FDC1004_ManualTest(I2C_HandleTypeDef *hi2c);

#ifdef __cplusplus
}
#endif

#endif  //  FDC1004_MANUAL_TEST_H
