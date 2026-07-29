
#pragma once
#include <cstdint>

const size_t PRESSURE_HPE_WINDOW_SIZE = 10;
const size_t PRESSURE_ETA_WINDOW_SIZE = 10;

// If changed, please update pipeline
const size_t PRESSURE_ETA_NUMBER_SENSORS = 3;

const double PRESSURE_ETA_MIN_VALUE = 0.;
const double PRESSURE_ETA_MAX_VALUE = 80.;

// Keep two and remove the outlier.
const size_t PRESSURE_ETA_NUMBER_KEPT = 2;
// At least one of the sensors should work
// for the pressure data point to be logged. 
const size_t PRESSURE_ETA_MIN_NUMBER  = 1;

static const char* ETA1_NAME = "eta1";
static const char* ETA2_NAME = "eta2";
static const char* ETA3_NAME = "eta3";
static const char* HPE_NAME  = "hpe";
