extern "C" {
#include "main_app.h"
}

#include "stm32h7xx_hal.h"

#include "Modules/Sensors/impl/lox/fls.hpp"
#include "Data/propulsion/data.hpp"
#include <stdio.h>
#include "Modules/Sensors/impl/lox/pressure_ota.hpp"

#include "Drivers/SensataPte7300/SensataPte7300HardwareTest.hpp"

static FLSModule fls_module;
static PressureOtaSensorModule ota_module;



void prc::PropSensorsStoreLox::set_FLS(double fls) {
	printf("Set new FLS value of %lf\r\n", fls);
}

void prc::PropSensorsStoreLox::set_FLS_mean(double fls_mean) {
	printf("Set new FLS_mean value of %lf\r\n", fls_mean);

}

void prc::PropSensorsStoreLox::set_pressure_OTA1(double ota) {
	printf("Set new OTA1_pressure value of %lf\r\n", ota);
}

void prc::PropSensorsStoreLox::set_pressure_OTA2(double ota) {
	printf("Set new OTA2_pressure value of %lf\r\n", ota);
}

void prc::PropSensorsStoreLox::set_pressure_OTA3(double ota) {
	printf("Set new OTA3_pressure value of %lf\r\n", ota);
}

void prc::PropSensorsStoreLox::set_pressure_OTA_mean(double ota) {
	printf("Set new OTA_mean_pressure value of %lf\r\n", ota);
}


void main_init() {

}

void main_tick() {
	uint32_t tickStart = HAL_GetTick();
	pte7300_print_data();
	uint32_t tickEnd = HAL_GetTick();
	HAL_Delay(1000);
	// fls_module.tick();
	// ota_module.tick();
}
