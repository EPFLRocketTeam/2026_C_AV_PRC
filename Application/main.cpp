extern "C" {
#include "main_app.h"
}

#include "stm32h7xx_hal.h"

#include "Modules/Sensors/impl/lox/fls.hpp"
#include "Data/propulsion/data.hpp"
#include <stdio.h>
#include "Modules/Sensors/impl/lox/pressure_ota.hpp"
#include "Modules/Sensors/impl/engine/chamber.hpp"

#include "Drivers/SensataPte7300/SensataPte7300HardwareTest.hpp"
#include "../../Drivers/Valve/valve_manual_test.hpp"

static FLSModule fls_module;
static PressureOtaSensorModule ota_module;

static ChamberModule chamber;

void prc::PropSensorsStoreEngine::set_pressure_C(double pressure_C) {
	printf("p_C=%lf\r\n", pressure_C);
}
void prc::PropSensorsStoreEngine::set_pressure_C_mean(double pressure_C_mean) {
	printf("p_C_mean=%lf\r\n", pressure_C_mean);
}
void prc::PropSensorsStoreEngine::set_temperature_C(double temperature_C) {
	printf("t_C=%lf\r\n", temperature_C);
}
void prc::PropSensorsStoreEngine::set_temperature_C_mean(double temperature_C_mean) {
	printf("t_C_mean=%lf\r\n", temperature_C_mean);
}

void main_init() {

	Valve_ManualTest();

	chamber.init();
}

void main_tick() {
	printf("\r\n");
	chamber.tick();
	HAL_Delay(10);
	// fls_module.tick();
	// ota_module.tick();
}
