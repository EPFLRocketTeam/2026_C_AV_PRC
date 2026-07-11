//    FILE: FDC1004.cpp
//  AUTHOR: Ashwin Whitchurch / ProtoCentral (original Arduino library)
// ADAPTED: STM32 HAL port (I2C_HandleTypeDef), style matched to INA228.cpp
// PURPOSE: STM32 HAL driver for the FDC1004, I2C, 4-channel capacitance-to-digital converter.

#include "../FDC1004.h"

#include <cmath>


////////////////////////////////////////////////////////
//
//  CONSTRUCTOR
//
FDC1004::FDC1004(I2C_HandleTypeDef *i2c, const uint8_t address, const uint8_t rate)
{
  _i2c     = i2c;
  _address = address;
  _rate    = rate;
  _error   = 0;
}


bool FDC1004::begin()
{
  return isConnected();
}


bool FDC1004::isConnected()
{
  return HAL_I2C_IsDeviceReady(_i2c, _address << 1, 3, 1000) == HAL_OK;
}


uint8_t FDC1004::getAddress()
{
  return _address;
}


////////////////////////////////////////////////////////
//
//  MEASUREMENT
//
float FDC1004::getCapacitance(uint8_t channel)
{
  if (channel > FDC1004_CHANNEL_MAX)
  {
    _error = -3;
    return NAN;
  }

  //  measurement slot == channel number.
  uint8_t measurement = channel;
  uint8_t capdac = _capdac[channel];

  if (! _configureMeasurement(measurement, channel, capdac)) return NAN;
  if (! _triggerMeasurement(measurement, _rate)) return NAN;

  HAL_Delay(_measurementDelay());

  int16_t raw = 0;
  if (! _readMeasurement(measurement, &raw)) return NAN;

  //  auto-adjust CAPDAC for next call if measurement is out of range.
  if (raw > FDC1004_UPPER_BOUND && capdac < FDC1004_CAPDAC_MAX)
  {
    _capdac[channel] = capdac + 1;
  }
  else if (raw < FDC1004_LOWER_BOUND && capdac > 0)
  {
    _capdac[channel] = capdac - 1;
  }

  float capacitance_af = (float)FDC1004_ATTOFARADS_UPPER_WORD * (float)raw;
  float capacitance_pf = capacitance_af / 1000000.0f;
  capacitance_pf += ((float)FDC1004_FEMTOFARADS_CAPDAC * (float)capdac) / 1000.0f;
  return capacitance_pf;
}


////////////////////////////////////////////////////////
//
//  CONFIGURATION
//
bool FDC1004::setRate(uint8_t rate)
{
  if (rate != FDC1004_RATE_100HZ && rate != FDC1004_RATE_200HZ && rate != FDC1004_RATE_400HZ)
  {
    return false;
  }
  _rate = rate;
  return true;
}


uint8_t FDC1004::getRate()
{
  return _rate;
}


bool FDC1004::setCapdac(uint8_t channel, uint8_t capdac)
{
  if (channel > FDC1004_CHANNEL_MAX || capdac > FDC1004_CAPDAC_MAX) return false;
  _capdac[channel] = capdac;
  return true;
}


uint8_t FDC1004::getCapdac(uint8_t channel)
{
  if (channel > FDC1004_CHANNEL_MAX) return 0;
  return _capdac[channel];
}


////////////////////////////////////////////////////////
//
//  ERROR HANDLING
//
int FDC1004::getLastError()
{
  int e = _error;
  _error = 0;
  return e;
}


////////////////////////////////////////////////////////
//
//  PRIVATE
//
bool FDC1004::_configureMeasurement(uint8_t measurement, uint8_t channel, uint8_t capdac)
{
  static const uint8_t CONF_MEAS_REG[4] = {
      FDC1004_REG_CONF_MEAS1, FDC1004_REG_CONF_MEAS2,
      FDC1004_REG_CONF_MEAS3, FDC1004_REG_CONF_MEAS4};

  uint16_t configuration_data = 0;
  configuration_data |= ((uint16_t)channel) << FDC1004_CONF_MEAS_CHA_SHIFT;

  if (capdac > 0)
  {
    configuration_data |= FDC1004_CONF_MEAS_CHB_CAPDAC << FDC1004_CONF_MEAS_CHB_SHIFT;
  }
  else
  {
    configuration_data |= FDC1004_CONF_MEAS_CHB_DISABLED << FDC1004_CONF_MEAS_CHB_SHIFT;
  }
  configuration_data |= ((uint16_t)capdac) << FDC1004_CONF_MEAS_CAPDAC_SHIFT;

  return _writeRegister(CONF_MEAS_REG[measurement], configuration_data);
}


bool FDC1004::_triggerMeasurement(uint8_t measurement, uint8_t rate)
{
  uint16_t trigger_data = 0;
  trigger_data |= ((uint16_t)rate) << FDC1004_FDC_CONF_RATE_SHIFT;
  trigger_data |= (1 << (7 - measurement));

  return _writeRegister(FDC1004_REG_FDC_CONF, trigger_data);
}


bool FDC1004::_readMeasurement(uint8_t measurement, int16_t *value)
{
  static const uint8_t MEAS_MSB_REG[4] = {
      FDC1004_REG_MEAS1_MSB, FDC1004_REG_MEAS2_MSB,
      FDC1004_REG_MEAS3_MSB, FDC1004_REG_MEAS4_MSB};

  uint16_t fdc_conf = 0;
  if (! _readRegister(FDC1004_REG_FDC_CONF, &fdc_conf)) return false;

  if (! (fdc_conf & (1 << (3 - measurement))))
  {
    _error = -4;  //  measurement not ready
    return false;
  }

  uint16_t msb = 0;
  if (! _readRegister(MEAS_MSB_REG[measurement], &msb)) return false;

  *value = (int16_t)msb;
  return true;
}


uint8_t FDC1004::_measurementDelay()
{
  switch (_rate)
  {
  case FDC1004_RATE_200HZ:  return 11;
  case FDC1004_RATE_400HZ:  return  6;
  case FDC1004_RATE_100HZ:
  default:                  return 11;
  }
}


bool FDC1004::_readRegister(uint8_t reg, uint16_t *value)
{
  _error = 0;
  uint8_t buf[2] = {0};

  if (HAL_I2C_Master_Transmit(_i2c, _address << 1, &reg, 1, 10) != HAL_OK)
  {
    _error = -1;
    return false;
  }
  if (HAL_I2C_Master_Receive(_i2c, (_address << 1) | 1, buf, 2, 10) != HAL_OK)
  {
    _error = -2;
    return false;
  }

  *value = ((uint16_t)buf[0] << 8) | buf[1];
  return true;
}


bool FDC1004::_writeRegister(uint8_t reg, uint16_t value)
{
  _error = 0;
  uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF) };

  if (HAL_I2C_Master_Transmit(_i2c, _address << 1, buf, 3, 10) != HAL_OK)
  {
    _error = -1;
    return false;
  }
  return true;
}


//  -- END OF FILE --
