// The manual MO, ME and igniter commands (Prc_Engine_ManualSet*, used by the
// USB shell) act only in Idle, and the engine FSM takes the outputs back as
// soon as it leaves Idle: ClearToIgnite closes MO and ME and switches the
// igniter off, every abort state switches the igniter off. Same host shim as
// test_engine_state_igniter.cpp.

#include <gtest/gtest.h>

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
static int g_igniter_level = -1;  // -1 until something writes PB2

uint32_t HAL_GetTick(void) { return g_now_ms; }
void HAL_GPIO_WritePin(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState state) {
  if (port == Igniter_GPIO_Port && pin == Igniter_Pin) g_igniter_level = (state == GPIO_PIN_SET);
}

extern "C" uint32_t app_timebase_now_ms(void) { return g_now_ms; }
extern "C" uint64_t app_timebase_now_us(void) { return uint64_t(g_now_ms) * 1000u; }
extern "C" bool app_printf_is_enabled() { return false; }

class FakeValve : public IValve {
public:
  explicit FakeValve(const char* name) : name_(name) {}
  ValveStatus open() override { open_ = true; return ValveStatus::Ok; }
  ValveStatus close() override { open_ = false; return ValveStatus::Ok; }
  bool is_open() const override { return open_; }
  const char* name() const override { return name_; }
private:
  const char* name_;
  bool open_ = false;
};

static FakeValve g_mo("MO");
static FakeValve g_me("ME");

IValve* Valve_Get(ValveId id) {
  if (id == k_valve_mo) return &g_mo;
  if (id == k_valve_me) return &g_me;
  return nullptr;
}
void Prc_Can_SendPreburnLox() {}
void Prc_Can_SendPreburnFuel() {}

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

// ctest runs every TEST in its own process, so each one starts in Idle.

TEST(EngineStateManual, IgniterOnByHandInIdleIsSwitchedOffOnClearToIgnite) {
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::Idle);
  ASSERT_TRUE(Prc_Engine_ManualSetIgniter(true));
  ASSERT_EQ(g_igniter_level, 1);

  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::ClearToIgnite);
  EXPECT_EQ(g_igniter_level, 0);
}

TEST(EngineStateManual, MoAndMeOpenedByHandAreClosedOnClearToIgnite) {
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::Idle);
  ASSERT_TRUE(Prc_Engine_ManualSetMo(true));
  ASSERT_TRUE(Prc_Engine_ManualSetMe(true));
  ASSERT_TRUE(g_mo.is_open());
  ASSERT_TRUE(g_me.is_open());

  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::ClearToIgnite);
  EXPECT_FALSE(g_mo.is_open());
  EXPECT_FALSE(g_me.is_open());
}

TEST(EngineStateManual, ManualCommandsAreRefusedOnceTheFsmLeftIdle) {
  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::ClearToIgnite);

  EXPECT_FALSE(Prc_Engine_ManualSetIgniter(true));
  EXPECT_FALSE(Prc_Engine_ManualSetMo(true));
  EXPECT_FALSE(Prc_Engine_ManualSetMe(true));
  EXPECT_EQ(g_igniter_level, 0);  // only the ClearToIgnite entry wrote it
  EXPECT_FALSE(g_mo.is_open());
  EXPECT_FALSE(g_me.is_open());
}

TEST(EngineStateManual, IdleOnlyGateRefusesEveryOtherState) {
  const int last = static_cast<int>(EngineState::AbortInFlightEthanol);
  for (int i = 0; i <= last; i++) {
    const auto state = static_cast<EngineState>(i);
    EXPECT_EQ(Prc_Engine_ManualAllowedIn(state), state == EngineState::Idle)
        << PrcEngineState::stateToString(state);
    EXPECT_NE(PrcEngineState::stateToString(state), "ERROR") << "enum value " << i;
  }
}

TEST(EngineStateManual, GroundAbortStatesForceTheIgniterOff) {
  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::ClearToIgnite);

  // Pretend the igniter was left on, then check each abort entry writes it off.
  g_igniter_level = 1;
  Send(pi::constants::MessageId::broadcast_abort);
  Tick(1);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortOnGround);
  EXPECT_EQ(g_igniter_level, 0);

  g_igniter_level = 1;
  Tick(1);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortOnGroundOxydant);
  EXPECT_EQ(g_igniter_level, 0);

  g_igniter_level = 1;
  Tick(config::get().Burn.CutoffDelayMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortOnGroundEthanol);
  EXPECT_EQ(g_igniter_level, 0);

  // and the reset back to Idle switches it off too
  g_igniter_level = 1;
  Send(pi::constants::MessageId::prc_reset);
  Tick(1);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::Idle);
  EXPECT_EQ(g_igniter_level, 0);
}

TEST(EngineStateManual, InFlightAbortStatesForceTheIgniterOff) {
  Send(pi::constants::MessageId::prc_clear_to_ignite);
  Tick();
  Send(pi::constants::MessageId::prc_ignite);
  Tick();
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionPrechill);
  Tick(config::get().Ignition.PrechillDurationMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionIgniter);
  Tick(config::get().Ignition.IgniterDurationMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionBurnStartMe);
  Tick(config::get().Ignition.DelayMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::IgnitionBurnStartMo);
  Tick(config::get().Ignition.RampUpMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::Burn);

  g_igniter_level = 1;
  Send(pi::constants::MessageId::broadcast_abort);
  Tick(1);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortInFlight);
  EXPECT_EQ(g_igniter_level, 0);

  g_igniter_level = 1;
  Tick(1);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortInFlightOxydant);
  EXPECT_EQ(g_igniter_level, 0);

  g_igniter_level = 1;
  Tick(config::get().Burn.CutoffDelayMs);
  ASSERT_EQ(Prc_Engine_Fsm_GetState(), EngineState::AbortInFlightEthanol);
  EXPECT_EQ(g_igniter_level, 0);
}
