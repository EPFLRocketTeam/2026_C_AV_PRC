//    FILE: FDC1004.h
//  AUTHOR: Ashwin Whitchurch / ProtoCentral (original Arduino library)
// ADAPTED: STM32 HAL port (I2C_HandleTypeDef), style matched to INA228.h
// PURPOSE: STM32 HAL driver for the FDC1004, I2C, 4-channel capacitance-to-digital converter.
//     URL: https://github.com/protocentral/ProtoCentral_fdc1004_breakout

#ifndef FDC1004_H
#define FDC1004_H

#include "stm32h7xx_hal.h"

#include <cstdint>

#define FDC1004_I2C_ADDRESS         0x50

//      REGISTERS                   ADDRESS   BITS  RW
#define FDC1004_REG_MEAS1_MSB       0x00   //  16    R-
#define FDC1004_REG_MEAS1_LSB       0x01   //  16    R-
#define FDC1004_REG_MEAS2_MSB       0x02   //  16    R-
#define FDC1004_REG_MEAS2_LSB       0x03   //  16    R-
#define FDC1004_REG_MEAS3_MSB       0x04   //  16    R-
#define FDC1004_REG_MEAS3_LSB       0x05   //  16    R-
#define FDC1004_REG_MEAS4_MSB       0x06   //  16    R-
#define FDC1004_REG_MEAS4_LSB       0x07   //  16    R-
#define FDC1004_REG_CONF_MEAS1      0x08   //  16    RW
#define FDC1004_REG_CONF_MEAS2      0x09   //  16    RW
#define FDC1004_REG_CONF_MEAS3      0x0A   //  16    RW
#define FDC1004_REG_CONF_MEAS4      0x0B   //  16    RW
#define FDC1004_REG_FDC_CONF        0x0C   //  16    RW
#define FDC1004_REG_DEVICE_ID       0xFF   //  16    R-

//  CONF_MEASx bit shifts
#define FDC1004_CONF_MEAS_CHA_SHIFT     13
#define FDC1004_CONF_MEAS_CHB_SHIFT     10
#define FDC1004_CONF_MEAS_CAPDAC_SHIFT  5
#define FDC1004_CONF_MEAS_CHB_DISABLED  0x7
#define FDC1004_CONF_MEAS_CHB_CAPDAC    0x4

//  FDC_CONF bit shifts
#define FDC1004_FDC_CONF_RATE_SHIFT     10
#define FDC1004_FDC_CONF_REPEAT_SHIFT   8

//  Sample rates
#define FDC1004_RATE_100HZ   0x01
#define FDC1004_RATE_200HZ   0x02
#define FDC1004_RATE_400HZ   0x03

#define FDC1004_CAPDAC_MAX   0x1F
#define FDC1004_CHANNEL_MAX  0x03

//  measurement bounds that indicate CAPDAC needs adjusting
#define FDC1004_UPPER_BOUND  0x4000
#define FDC1004_LOWER_BOUND  (-0x4000)

//  conversion constants (see datasheet)
#define FDC1004_ATTOFARADS_UPPER_WORD  457
#define FDC1004_FEMTOFARADS_CAPDAC     3028


class FDC1004
{
public:
  explicit FDC1004(I2C_HandleTypeDef *i2c, const uint8_t address = FDC1004_I2C_ADDRESS,
                    const uint8_t rate = FDC1004_RATE_100HZ);

  bool     begin();
  bool     isConnected();
  uint8_t  getAddress();

  //  full measurement cycle (configure + trigger + wait + read), auto-adjusts CAPDAC.
  //  returns NAN on error (check getLastError()).
  float    getCapacitance(uint8_t channel);

  bool     setRate(uint8_t rate);
  uint8_t  getRate();

  bool     setCapdac(uint8_t channel, uint8_t capdac);
  uint8_t  getCapdac(uint8_t channel);

  int      getLastError();

private:
  uint8_t  _address;
  I2C_HandleTypeDef *_i2c;
  uint8_t  _rate;
  uint8_t  _capdac[4] = {0, 0, 0, 0};
  int      _error;

  bool     _configureMeasurement(uint8_t measurement, uint8_t channel, uint8_t capdac);
  bool     _triggerMeasurement(uint8_t measurement, uint8_t rate);
  bool     _readMeasurement(uint8_t measurement, int16_t *value);
  uint8_t  _measurementDelay();

  bool     _readRegister(uint8_t reg, uint16_t *value);
  bool     _writeRegister(uint8_t reg, uint16_t value);
};

#endif  //  FDC1004_H
