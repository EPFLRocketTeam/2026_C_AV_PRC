//    FILE: FDC1004_manual_test.cpp
// PURPOSE: C-callable smoke test for the FDC1004 driver, for use from main.c.

#include "../FDC1004_manual_test.h"
#include "../FDC1004.h"

#include <cmath>
#include <cstdio>

volatile int   fdc_connected  = 0;
volatile float fdc_cap_ch0_pf = 0.0f;
volatile float fdc_cap_ch1_pf = 0.0f;
volatile float fdc_cap_ch2_pf = 0.0f;
volatile float fdc_cap_ch3_pf = 0.0f;
volatile int   fdc_last_error = 0;

// Streams one CSV-tagged line per call: FDC1004,<tick_ms>,<ch0>,<ch1>,<ch2>,<ch3>
// The "FDC1004," prefix lets a host-side script pick this line out of
// everything else sharing the same serial output (other tests' logs, the
// heartbeat, etc.) instead of relying on print ordering. Channels are NAN
// when the sensor isn't connected, so a disconnect shows up as NaN rows in
// the log instead of silently repeating the last good value.
void FDC1004_ManualTest(I2C_HandleTypeDef *hi2c)
{
  //  static: constructed once, so _capdac[] persists (and can converge)
  //  across the repeated calls made from the main while(1) loop.
  static FDC1004 fdc(hi2c);

  fdc_connected = fdc.begin() ? 1 : 0;

  if (fdc_connected)
  {
    fdc_cap_ch0_pf = fdc.getCapacitance(0);
    fdc_cap_ch1_pf = fdc.getCapacitance(1);
    fdc_cap_ch2_pf = fdc.getCapacitance(2);
    fdc_cap_ch3_pf = fdc.getCapacitance(3);
    fdc_last_error = fdc.getLastError();
  }
  else
  {
    fdc_cap_ch0_pf = fdc_cap_ch1_pf = fdc_cap_ch2_pf = fdc_cap_ch3_pf = NAN;
    fdc_last_error = -1;
  }

  printf("FDC1004,%lu,%.4f,%.4f,%.4f,%.4f\r\n",
         (unsigned long)HAL_GetTick(),
         static_cast<double>(fdc_cap_ch0_pf),
         static_cast<double>(fdc_cap_ch1_pf),
         static_cast<double>(fdc_cap_ch2_pf),
         static_cast<double>(fdc_cap_ch3_pf));
}
