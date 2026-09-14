
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

static const char* OTA4_NAME = "ota4";
static const char* OTA5_NAME = "ota5";

inline constexpr float OTA_RREF_OHMS4 = 968.0f;
inline constexpr float OTA_RREF_OHMS5 = 952.0f;
