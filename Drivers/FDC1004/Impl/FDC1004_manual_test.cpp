//    FILE: FDC1004_manual_test.cpp
// PURPOSE: C-callable smoke test for the FDC1004 driver, for use from main.c.

#include "../FDC1004_manual_test.h"
#include "../FDC1004.h"

#include <cstdio>

volatile int   fdc_connected  = 0;
volatile float fdc_cap_ch0_pf = 0.0f;
volatile float fdc_cap_ch1_pf = 0.0f;
volatile float fdc_cap_ch2_pf = 0.0f;
volatile float fdc_cap_ch3_pf = 0.0f;
volatile int   fdc_last_error = 0;

void FDC1004_ManualTest(I2C_HandleTypeDef *hi2c)
{
  printf("[FDC1004] Manual test started\r\n");

  //  static: constructed once, so _capdac[] persists (and can converge)
  //  across the repeated calls made from the main while(1) loop.
  static FDC1004 fdc(hi2c);

  fdc_connected = fdc.begin() ? 1 : 0;
  printf("[FDC1004] begin(): %s\r\n", fdc_connected ? "connected" : "NOT connected");

  if (fdc_connected)
  {
    fdc_cap_ch0_pf = fdc.getCapacitance(0);
    fdc_last_error = fdc.getLastError();
    printf("[FDC1004] ch0 capacitance: %.3f pF (capdac=%u, last_error=%d)\r\n",
           fdc_cap_ch0_pf, fdc.getCapdac(0), fdc_last_error);

    fdc_cap_ch1_pf = fdc.getCapacitance(1);
    fdc_last_error = fdc.getLastError();
    printf("[FDC1004] ch1 capacitance: %.3f pF (capdac=%u, last_error=%d)\r\n",
           fdc_cap_ch1_pf, fdc.getCapdac(1), fdc_last_error);

    fdc_cap_ch2_pf = fdc.getCapacitance(2);
    fdc_last_error = fdc.getLastError();
    printf("[FDC1004] ch2 capacitance: %.3f pF (capdac=%u, last_error=%d)\r\n",
           fdc_cap_ch2_pf, fdc.getCapdac(2), fdc_last_error);

    fdc_cap_ch3_pf = fdc.getCapacitance(3);
    fdc_last_error = fdc.getLastError();
    printf("[FDC1004] ch3 capacitance: %.3f pF (capdac=%u, last_error=%d)\r\n",
           fdc_cap_ch3_pf, fdc.getCapdac(3), fdc_last_error);
  }

  printf("[FDC1004] Manual test done\r\n");
}
