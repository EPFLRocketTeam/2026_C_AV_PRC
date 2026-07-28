
#pragma once
#include <cstdint>

const size_t PRESSURE_HPO_WINDOW_SIZE = 10;
const size_t PRESSURE_OTA_WINDOW_SIZE = 10;
const size_t TEMPERATURE_OTA_WINDOW_SIZE = 10;
const size_t FLS_WINDOW_SIZE = 10;

// If changed, please update pipeline
const size_t PRESSURE_OTA_NUMBER_SENSORS = 3;

const double PRESSURE_OTA_MIN_VALUE = 0.;
const double PRESSURE_OTA_MAX_VALUE = 80.;

// Keep two and remove the outlier.
const size_t PRESSURE_OTA_NUMBER_KEPT = 2;
// At least one of the sensors should work
// for the pressure data point to be logged. 
const size_t PRESSURE_OTA_MIN_NUMBER  = 1;
