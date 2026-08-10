extern "C" {
#include "main_app.h"
}

#include "stm32h7xx_hal.h"

#include "Application/Data/data.hpp"
#include "Data/propulsion/data.hpp"
#include <stdio.h>

#include "Modules/Sensors/impl/lox/fls.hpp"
#include "Modules/Sensors/impl/lox/pressure_ota.hpp"
#include "Modules/Sensors/impl/lox/pressure_hpo.hpp"
#include "Modules/Sensors/impl/lox/temperature_ota.hpp"
#include "Modules/Sensors/impl/eth/pressure_eta.hpp"
#include "Modules/Sensors/impl/eth/pressure_hpe.hpp"
#include "Modules/Sensors/impl/engine/chamber.hpp"
#include "Modules/Sensors/impl/engine/oin.hpp"
#include "Modules/Sensors/impl/engine/ein.hpp"

#include "Drivers/SensataPte7300/SensataPte7300HardwareTest.hpp"
#include "../../Drivers/Valve/valve_manual_test.hpp"

// ---------------------------------------------------------------------------
// One sensor module instance per row of the sensor table (see project docs:
// "List of all of the Sensors"). Every board's firmware image is compiled
// with all three bays' modules -- main_tick() below only ticks the subset
// that belongs to this board's latched BoardRole, so a board only ever
// polls/reports the sensors its physical bay actually has wired.
// ---------------------------------------------------------------------------

// ── Engine bay ───────────────────────────────────────────────────────────
static ChamberModule chamber;        // P-C, T-C  (B3 Chamber, Sensata PTE7300)
static OxidizerInModule oin;         // P-OIN     (B3 Sensor Plate, Kulite/Sensata)
static EthanolInModule ein;          // P-EIN     (B3 Sensor Plate, Sensata PTE7300)
static TemperatureOinModule t_oin;   // T-OIN     (Lox Injector, PT1000)
static TemperatureEinModule t_ein;   // T-EIN     (B3 Sensor Plate, PT1000)

// ── Pressurant bay 1 (Fat Bay, PRC-Lox) ─────────────────────────────────
static PressureOtaSensorModule ota_module;   // P-OTA{1,2,3} (Lox Tank Ullage, Sensata PTE7300)
static PressureHpoSensorModule pressure_hpo; // P-HPO        (COPV 1, Sensata PTE7300)
static TemperatureOtaSensorModule1 t_ota1;   // T-OTA1       (Lox Tank, PT1000)
static TemperatureOtaSensorModule2 t_ota2;   // T-OTA2       (Lox Tank, PT1000)
static TemperatureOtaSensorModule3 t_ota3;   // T-OTA3       (Lox Tank, PT1000)
static TemperatureOtaSensorModule4 t_ota4;   // T-OTA4       (Lox Tank, PT1000)
static FLSModule fls_module;                 // FLS          (Inside Lox Tank, capacitive)

// ── Pressurant bay 2 (Skinny Bay, PRC-ETH) ──────────────────────────────
static PressureEtaSensorModule eta_module;   // P-ETA{1,2,3} (Eth Tank Ullage, Sensata PTE7300)
static PressureHpeSensorModule pressure_hpe; // P-HPE        (COPV 2, Sensata PTE7300)

// ---------------------------------------------------------------------------
// Engine bay setters
// ---------------------------------------------------------------------------

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
void prc::PropSensorsStoreEngine::set_pressure_OIN(double pressure_OIN) {
	printf("p_OIN=%lf\r\n", pressure_OIN);
}
void prc::PropSensorsStoreEngine::set_pressure_OIN_mean(double pressure_OIN_mean) {
	printf("p_OIN_mean=%lf\r\n", pressure_OIN_mean);
}
void prc::PropSensorsStoreEngine::set_pressure_EIN(double pressure_EIN) {
	printf("p_EIN=%lf\r\n", pressure_EIN);
}
void prc::PropSensorsStoreEngine::set_pressure_EIN_mean(double pressure_EIN_mean) {
	printf("p_EIN_mean=%lf\r\n", pressure_EIN_mean);
}
void prc::PropSensorsStoreEngine::set_temperature_OIN(double temperature_OIN) {
	printf("t_OIN=%lf\r\n", temperature_OIN);
}
void prc::PropSensorsStoreEngine::set_temperature_OIN_mean(double temperature_OIN_mean) {
	printf("t_OIN_mean=%lf\r\n", temperature_OIN_mean);
}
void prc::PropSensorsStoreEngine::set_temperature_EIN(double temperature_EIN) {
	printf("t_EIN=%lf\r\n", temperature_EIN);
}
void prc::PropSensorsStoreEngine::set_temperature_EIN_mean(double temperature_EIN_mean) {
	printf("t_EIN_mean=%lf\r\n", temperature_EIN_mean);
}

// ---------------------------------------------------------------------------
// Pressurant bay 1 (Lox) setters
// ---------------------------------------------------------------------------

void prc::PropSensorsStoreLox::set_pressure_OTA1(double pressure_OTA1) {
	printf("p_OTA1=%lf\r\n", pressure_OTA1);
}
void prc::PropSensorsStoreLox::set_pressure_OTA2(double pressure_OTA2) {
	printf("p_OTA2=%lf\r\n", pressure_OTA2);
}
void prc::PropSensorsStoreLox::set_pressure_OTA3(double pressure_OTA3) {
	printf("p_OTA3=%lf\r\n", pressure_OTA3);
}
void prc::PropSensorsStoreLox::set_pressure_OTA_mean(double pressure_OTA_mean) {
	printf("p_OTA_mean=%lf\r\n", pressure_OTA_mean);
}
void prc::PropSensorsStoreLox::set_pressure_HPO(double pressure_HPO) {
	printf("p_HPO=%lf\r\n", pressure_HPO);
}
void prc::PropSensorsStoreLox::set_pressure_HPO_mean(double pressure_HPO_mean) {
	printf("p_HPO_mean=%lf\r\n", pressure_HPO_mean);
}
void prc::PropSensorsStoreLox::set_temperature_OTA1(double temperature_OTA1) {
	printf("t_OTA1=%lf\r\n", temperature_OTA1);
}
void prc::PropSensorsStoreLox::set_temperature_OTA1_mean(double temperature_OTA1_mean) {
	printf("t_OTA1_mean=%lf\r\n", temperature_OTA1_mean);
}
void prc::PropSensorsStoreLox::set_temperature_OTA2(double temperature_OTA2) {
	printf("t_OTA2=%lf\r\n", temperature_OTA2);
}
void prc::PropSensorsStoreLox::set_temperature_OTA2_mean(double temperature_OTA2_mean) {
	printf("t_OTA2_mean=%lf\r\n", temperature_OTA2_mean);
}
void prc::PropSensorsStoreLox::set_temperature_OTA3(double temperature_OTA3) {
	printf("t_OTA3=%lf\r\n", temperature_OTA3);
}
void prc::PropSensorsStoreLox::set_temperature_OTA3_mean(double temperature_OTA3_mean) {
	printf("t_OTA3_mean=%lf\r\n", temperature_OTA3_mean);
}
void prc::PropSensorsStoreLox::set_temperature_OTA4(double temperature_OTA4) {
	printf("t_OTA4=%lf\r\n", temperature_OTA4);
}
void prc::PropSensorsStoreLox::set_temperature_OTA4_mean(double temperature_OTA4_mean) {
	printf("t_OTA4_mean=%lf\r\n", temperature_OTA4_mean);
}
void prc::PropSensorsStoreLox::set_FLS(double FLS) {
	printf("FLS=%lf\r\n", FLS);
}
void prc::PropSensorsStoreLox::set_FLS_mean(double FLS_mean) {
	printf("FLS_mean=%lf\r\n", FLS_mean);
}

// ---------------------------------------------------------------------------
// Pressurant bay 2 (Eth) setters
// ---------------------------------------------------------------------------

void prc::PropSensorsStoreEth::set_pressure_ETA1(double pressure_ETA1) {
	printf("p_ETA1=%lf\r\n", pressure_ETA1);
}
void prc::PropSensorsStoreEth::set_pressure_ETA2(double pressure_ETA2) {
	printf("p_ETA2=%lf\r\n", pressure_ETA2);
}
void prc::PropSensorsStoreEth::set_pressure_ETA3(double pressure_ETA3) {
	printf("p_ETA3=%lf\r\n", pressure_ETA3);
}
void prc::PropSensorsStoreEth::set_pressure_ETA_mean(double pressure_ETA_mean) {
	printf("p_ETA_mean=%lf\r\n", pressure_ETA_mean);
}
void prc::PropSensorsStoreEth::set_pressure_HPE(double pressure_HPE) {
	printf("p_HPE=%lf\r\n", pressure_HPE);
}
void prc::PropSensorsStoreEth::set_pressure_HPE_mean(double pressure_HPE_mean) {
	printf("p_HPE_mean=%lf\r\n", pressure_HPE_mean);
}

// ---------------------------------------------------------------------------

void main_init() {
	Valve_ManualTest();

	// init() just constructs driver/config objects (no bus traffic), so it's
	// harmless to run for every bay's sensors regardless of which one this
	// board turns out to be -- BoardRole isn't latched yet at this point
	// (Prc_Fsm_Init() runs after main_init(), see Core/Src/main.c), only
	// main_tick() below can gate on role.
	chamber.init();
	oin.init();
	ein.init();
	t_oin.init();
	t_ein.init();

	ota_module.init();
	pressure_hpo.init();
	t_ota1.init();
	t_ota2.init();
	t_ota3.init();
	t_ota4.init();
	fls_module.init();

	eta_module.init();
	pressure_hpe.init();
}

void main_tick() {
	printf("\r\n");

	switch (prc::PrcStore::get_instance().boardIdentityStore.get_role()) {
		case prc::BoardRole::EngineBay:
			chamber.tick();
			oin.tick();
			ein.tick();
			t_oin.tick();
			t_ein.tick();
			break;

		case prc::BoardRole::DprLox:
			ota_module.tick();
			pressure_hpo.tick();
			t_ota1.tick();
			t_ota2.tick();
			t_ota3.tick();
			t_ota4.tick();
			fls_module.tick();
			break;

		case prc::BoardRole::DprEth:
			eta_module.tick();
			pressure_hpe.tick();
			break;

		case prc::BoardRole::Unknown:
		default:
			// Role detection hasn't latched yet (or failed) -- report nothing
			// rather than guessing which bay's sensors to poll.
			break;
	}

	HAL_Delay(10);
}
