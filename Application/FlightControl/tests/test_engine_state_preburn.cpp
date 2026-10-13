// Drives the engine FSM through Prc_Engine_Fsm_Tick() with a LOX preburn
// shorter than the ignition delay, so the LOX preburn falls into
// IGNITION_BURN_START_ME, and checks each preburn is sent exactly once.

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>

#include "Application/FlightControl/engine_state.h"
#include "Application/FlightControl/prc_can.hpp"
#include "Application/app_timebase.h"
#include "Application/app_printf.h"
#include "Application/Config/config.hpp"
#include "Drivers/Valve/ValveList.hpp"
#include "Drivers/Plume/plume_storage.hpp"
#include "Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include/prc_intranet/const.hpp"
#include "Modules/Sensors/impl/common.hpp"

#include "main.h"

// ---------------------------------------------------------------------------
// Host stubs for what engine_state.cpp links against on the board
// ---------------------------------------------------------------------------

GPIO_TypeDef test_gpiob;

static uint32_t g_now_ms = 0;
static int g_preburn_lox_count = 0;
static int g_preburn_fuel_count = 0;

uint32_t HAL_GetTick(void) { return g_now_ms; }
void HAL_GPIO_WritePin(GPIO_TypeDef*, uint16_t, GPIO_PinState) {}

extern "C" uint32_t app_timebase_now_ms(void) { return g_now_ms; }
extern "C" uint64_t app_timebase_now_us(void) { return uint64_t(g_now_ms) * 1000u; }
extern "C" bool app_printf_is_enabled() { return false; }

IValve* Valve_Get(ValveId) { return nullptr; }
void Prc_Can_SendPreburnLox() { g_preburn_lox_count++; }
void Prc_Can_SendPreburnFuel() { g_preburn_fuel_count++; }

void SDCardInterface::beginTransaction() {}
void SDCardInterface::endTransaction() {}
uint8_t SDCardInterface::write(const uint8_t*, int) { return 0; }

static SDCardInterface g_sd;
static PlumeStorage g_storage(&g_sd);
static EngineDataLogger<PlumeStorage> g_engine_logger(g_storage);
EngineDataLogger<PlumeStorage>& getEngineLogger() { return g_engine_logger; }

// ---------------------------------------------------------------------------

namespace pi = prc_intranet;

static void Send(pi::constants::MessageId id) {
  prc::PrcStore::get_instance().intranetCmdStore.set_id(static_cast<uint16_t>(id));
}

static void Tick(uint32_t advance_ms = 0) {
  g_now_ms += advance_ms;
  Prc_Engine_Fsm_Tick();
}

// Uploads a LOX preburn duration through the same put/commit path the
// flight params use over CAN.
static void SetPreburnDurationLoxMs(float value) {
  const size_t offset = offsetof(FlightParams, Pressurization)
                      + offsetof(PressurizationParams, PreburnDurationLoxMs);
  uint8_t bytes[sizeof(value)];
  std::memcpy(bytes, &value, sizeof(value));
  config::internal::put(static_cast<uint8_t>(offset), bytes, sizeof(bytes));
  config::internal::commit();
}

TEST(EngineStatePreburn, LoxPreburnDuringBurnStartMeSendsLoxOnce) {
  // Shorter than Ignition.DelayMs (109 ms), so the LOX send point lies
  // past the end of IGNITION_IGNITER and falls into IGNITION_BURN_START_ME.
  SetPreburnDurationLoxMs(50.f);
  const auto& ignition = config::get().Ignition;
  const auto& pressurization = config::get().Pressurization;
  ASSERT_EQ(pressurization.PreburnDurationLoxMs, 50.f);

  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  Send(pi::constants::MessageId::prc_ignite);
  Tick();
  Tick(ignition.PrechillDurationMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionIgniter);

  // Fuel preburn point inside the igniter phase.
  const uint32_t fuel_at = ignition.IgniterDurationMs - uint32_t(pressurization.PreburnDurationFuelMs);
  Tick(fuel_at);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionIgniter);
  EXPECT_EQ(g_preburn_fuel_count, 1);
  EXPECT_EQ(g_preburn_lox_count, 0);

  Tick(ignition.IgniterDurationMs - fuel_at);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionBurnStartMe);

  // LOX preburn point inside BURN_START_ME.
  const uint32_t lox_at = ignition.DelayMs - uint32_t(pressurization.PreburnDurationLoxMs);
  Tick(lox_at);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionBurnStartMe);

  Tick(ignition.DelayMs - lox_at);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionBurnStartMo);

  EXPECT_EQ(g_preburn_lox_count, 1);
  EXPECT_EQ(g_preburn_fuel_count, 1);
}
