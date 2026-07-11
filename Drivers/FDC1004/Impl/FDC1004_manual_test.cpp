//    FILE: FDC1004_manual_test.cpp
// PURPOSE: C-callable smoke test for the FDC1004 driver, for use from main.c.

#include "../FDC1004_manual_test.h"
#include "../FDC1004.h"

volatile int   fdc_connected  = 0;
volatile float fdc_cap_ch0_pf = 0.0f;
volatile int   fdc_last_error = 0;

void FDC1004_ManualTest(I2C_HandleTypeDef *hi2c)
{
  FDC1004 fdc(hi2c);

  fdc_connected = fdc.begin() ? 1 : 0;
  if (fdc_connected)
  {
    fdc_cap_ch0_pf = fdc.getCapacitance(0);
    fdc_last_error = fdc.getLastError();
  }
}
