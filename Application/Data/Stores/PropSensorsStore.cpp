#include "Application/Data/data.hpp"

using namespace prc;

PropSensors::PropSensors()
:   pressure_C(0.0),
    pressure_C_mean(0.0),
    temperature_C(0.0),
    temperature_C_mean(0.0),
    pressure_OIN(0.0),
    pressure_OIN_mean(0.0),
    pressure_EIN(0.0),
    pressure_EIN_mean(0.0),
    temperature_EIN(0.0),
    temperature_EIN_mean(0.0),
    temperature_OIN(0.0),
    temperature_OIN_mean(0.0),
    pressure_OTA(0.0),
    pressure_OTA_mean(0.0),
    pressure_HPO(0.0),
    pressure_HPO_mean(0.0),
    temperature_OTA{0.0, 0.0, 0.0, 0.0},
    temperature_OTA_mean{0.0, 0.0, 0.0, 0.0},
    FLS(0.0),
    FLS_mean(0.0),
    pressure_ETA(0.0),
    pressure_ETA_mean(0.0),
    pressure_HPE(0.0),
    pressure_HPE_mean(0.0)
{}

PropSensorsStore::PropSensorsStore() {}

double PropSensorsStore::get_pressure_C() const { return data_.pressure_C; }
void PropSensorsStore::set_pressure_C(double value) { data_.pressure_C = value; }
double PropSensorsStore::get_pressure_C_mean() const { return data_.pressure_C_mean; }
void PropSensorsStore::set_pressure_C_mean(double value) { data_.pressure_C_mean = value; }

double PropSensorsStore::get_temperature_C() const { return data_.temperature_C; }
void PropSensorsStore::set_temperature_C(double value) { data_.temperature_C = value; }
double PropSensorsStore::get_temperature_C_mean() const { return data_.temperature_C_mean; }
void PropSensorsStore::set_temperature_C_mean(double value) { data_.temperature_C_mean = value; }

double PropSensorsStore::get_pressure_OIN() const { return data_.pressure_OIN; }
void PropSensorsStore::set_pressure_OIN(double value) { data_.pressure_OIN = value; }
double PropSensorsStore::get_pressure_OIN_mean() const { return data_.pressure_OIN_mean; }
void PropSensorsStore::set_pressure_OIN_mean(double value) { data_.pressure_OIN_mean = value; }

double PropSensorsStore::get_pressure_EIN() const { return data_.pressure_EIN; }
void PropSensorsStore::set_pressure_EIN(double value) { data_.pressure_EIN = value; }
double PropSensorsStore::get_pressure_EIN_mean() const { return data_.pressure_EIN_mean; }
void PropSensorsStore::set_pressure_EIN_mean(double value) { data_.pressure_EIN_mean = value; }

double PropSensorsStore::get_temperature_EIN() const { return data_.temperature_EIN; }
void PropSensorsStore::set_temperature_EIN(double value) { data_.temperature_EIN = value; }
double PropSensorsStore::get_temperature_EIN_mean() const { return data_.temperature_EIN_mean; }
void PropSensorsStore::set_temperature_EIN_mean(double value) { data_.temperature_EIN_mean = value; }

double PropSensorsStore::get_temperature_OIN() const { return data_.temperature_OIN; }
void PropSensorsStore::set_temperature_OIN(double value) { data_.temperature_OIN = value; }
double PropSensorsStore::get_temperature_OIN_mean() const { return data_.temperature_OIN_mean; }
void PropSensorsStore::set_temperature_OIN_mean(double value) { data_.temperature_OIN_mean = value; }

double PropSensorsStore::get_pressure_OTA() const { return data_.pressure_OTA; }
void PropSensorsStore::set_pressure_OTA(double value) { data_.pressure_OTA = value; }
double PropSensorsStore::get_pressure_OTA_mean() const { return data_.pressure_OTA_mean; }
void PropSensorsStore::set_pressure_OTA_mean(double value) { data_.pressure_OTA_mean = value; }

double PropSensorsStore::get_pressure_HPO() const { return data_.pressure_HPO; }
void PropSensorsStore::set_pressure_HPO(double value) { data_.pressure_HPO = value; }
double PropSensorsStore::get_pressure_HPO_mean() const { return data_.pressure_HPO_mean; }
void PropSensorsStore::set_pressure_HPO_mean(double value) { data_.pressure_HPO_mean = value; }

double PropSensorsStore::get_temperature_OTA(uint8_t sensor_index) const { return data_.temperature_OTA[sensor_index]; }
void PropSensorsStore::set_temperature_OTA(uint8_t sensor_index, double value) { data_.temperature_OTA[sensor_index] = value; }
double PropSensorsStore::get_temperature_OTA_mean(uint8_t sensor_index) const { return data_.temperature_OTA_mean[sensor_index]; }
void PropSensorsStore::set_temperature_OTA_mean(uint8_t sensor_index, double value) { data_.temperature_OTA_mean[sensor_index] = value; }

double PropSensorsStore::get_FLS() const { return data_.FLS; }
void PropSensorsStore::set_FLS(double value) { data_.FLS = value; }
double PropSensorsStore::get_FLS_mean() const { return data_.FLS_mean; }
void PropSensorsStore::set_FLS_mean(double value) { data_.FLS_mean = value; }

double PropSensorsStore::get_pressure_ETA() const { return data_.pressure_ETA; }
void PropSensorsStore::set_pressure_ETA(double value) { data_.pressure_ETA = value; }
double PropSensorsStore::get_pressure_ETA_mean() const { return data_.pressure_ETA_mean; }
void PropSensorsStore::set_pressure_ETA_mean(double value) { data_.pressure_ETA_mean = value; }

double PropSensorsStore::get_pressure_HPE() const { return data_.pressure_HPE; }
void PropSensorsStore::set_pressure_HPE(double value) { data_.pressure_HPE = value; }
double PropSensorsStore::get_pressure_HPE_mean() const { return data_.pressure_HPE_mean; }
void PropSensorsStore::set_pressure_HPE_mean(double value) { data_.pressure_HPE_mean = value; }
