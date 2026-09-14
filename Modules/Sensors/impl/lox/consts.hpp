
#pragma once
#include <cstdint>

const size_t PRESSURE_HPO_WINDOW_SIZE = 10;
const size_t PRESSURE_OTA_WINDOW_SIZE = 10;
const size_t FLS_WINDOW_SIZE = 10;

// If changed, please update pipeline
const size_t PRESSURE_OTA_NUMBER_SENSORS = 3;

const double PRESSURE_OTA_MIN_VALUE = -0.25;
const double PRESSURE_OTA_MAX_VALUE = 80.;

// Keep two and remove the outlier.
const size_t PRESSURE_OTA_NUMBER_KEPT = 2;
// At least one of the sensors should work
// for the pressure data point to be logged. 
const size_t PRESSURE_OTA_MIN_NUMBER  = 1;

const int PRESSURE_HPO_MULTIPLIER = 4;

static const char* OTA1_NAME = "ota1";
static const char* OTA2_NAME = "ota2";
static const char* OTA3_NAME = "ota3";
static const char* HPO_NAME  = "hpo";

inline constexpr float OTA_RREF_OHMS1 = 1100.0f; // it appears OTA1 is broken for now
inline constexpr float OTA_RREF_OHMS2 = 930.0f;
inline constexpr float OTA_RREF_OHMS3 = 916.0f;
