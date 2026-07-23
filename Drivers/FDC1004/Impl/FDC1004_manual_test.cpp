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
volatile int   fdc_err_ch[4]  = {0, 0, 0, 0};
volatile int   fdc_last_error = 0;

// Streams one CSV-tagged line per call:
//   FDC1004,<tick_ms>,<ch0>,<ch1>,<ch2>,<ch3>,<e0>,<e1>,<e2>,<e3>
// The "FDC1004," prefix lets a host-side script pick this line out of
// everything else sharing the same serial output (other tests' logs, the
// heartbeat, etc.) instead of relying on print ordering.
//
// A channel is NAN whenever its conversion is invalid, and the matching
// error column says why (see error code table in FDC1004.h):
//   -1/-2 I2C fault, -4 not ready, -5 clipped (CAPDAC still converging).
// Expect a few -5 rows per channel after power-up while CAPDAC walks up
// to the ~20 pF baseline (one step per call, 3.125 pF per step); that is
// normal and the values become valid once it converges.
void FDC1004_ManualTest(I2C_HandleTypeDef *hi2c)
{
  //  static: constructed once, so _capdac[] persists (and can converge)
  //  across the repeated calls made from the main while(1) loop.
  static FDC1004 fdc(hi2c);

  //  begin() also verifies manufacturer/device ID, so a wrong chip at
  //  0x50 (e.g. an EEPROM) now shows up as "not connected".
  fdc_connected = fdc.begin() ? 1 : 0;

  if (fdc_connected)
  {
    //  capture the error per channel, before the next call clears it.
    fdc_cap_ch0_pf = fdc.getCapacitance(0);
    fdc_err_ch[0]  = fdc.getLastError();
    fdc_cap_ch1_pf = fdc.getCapacitance(1);
    fdc_err_ch[1]  = fdc.getLastError();
    fdc_cap_ch2_pf = fdc.getCapacitance(2);
    fdc_err_ch[2]  = fdc.getLastError();
    fdc_cap_ch3_pf = fdc.getCapacitance(3);
    fdc_err_ch[3]  = fdc.getLastError();
  }
  else
  {
    fdc_cap_ch0_pf = fdc_cap_ch1_pf = fdc_cap_ch2_pf = fdc_cap_ch3_pf = NAN;
    fdc_err_ch[0] = fdc_err_ch[1] = fdc_err_ch[2] = fdc_err_ch[3] = fdc.getLastError();
  }

  //  first nonzero error, for a quick single-value debugger watch.
  fdc_last_error = 0;
  for (int i = 0; i < 4; i++)
  {
    if (fdc_err_ch[i] != 0)
    {
      fdc_last_error = fdc_err_ch[i];
      break;
    }
  }

  printf("FDC1004,%lu,%.4f,%.4f,%.4f,%.4f,%d,%d,%d,%d\r\n",
         (unsigned long)HAL_GetTick(),
         static_cast<double>(fdc_cap_ch0_pf),
         static_cast<double>(fdc_cap_ch1_pf),
         static_cast<double>(fdc_cap_ch2_pf),
         static_cast<double>(fdc_cap_ch3_pf),
         fdc_err_ch[0], fdc_err_ch[1], fdc_err_ch[2], fdc_err_ch[3]);
}
