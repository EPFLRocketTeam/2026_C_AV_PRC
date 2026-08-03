
#pragma once
#include <cstdint>

const size_t PRESSURE_C_WINDOW_SIZE   = 10;
const size_t PRESSURE_OIN_WINDOW_SIZE = 10;
const size_t PRESSURE_EIN_WINDOW_SIZE = 10;

const size_t TEMPERATURE_C_WINDOW_SIZE   = 10;
const size_t TEMPERATURE_OIN_WINDOW_SIZE = 10;
const size_t TEMPERATURE_EIN_WINDOW_SIZE = 10;


static const char* CHAMBER_NAME = "chamber";
static const char* OIN_NAME = "oin";
static const char* EIN_NAME = "ein";
static const char* TEIN_NAME = "t_ein";
static const char* TOIN_NAME = "t_oin";

#define PT1000_CHANNEL_T_EIN ADC_CHANNEL_2 // TODO: placeholder, confirm real ADC channel from schematic/.ioc
#define PT1000_CHANNEL_T_OIN ADC_CHANNEL_3 // TODO: placeholder, confirm real ADC channel from schematic/.ioc
