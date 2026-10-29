#include "Application/FlightControl/prc_commands.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "Application/FlightControl/prc_shell.hpp"
#include "Application/FlightControl/prc_state.h"
#include "Application/FlightControl/engine_state.h"
#include "Application/Data/data.hpp"
#include "Application/Config/config.hpp"
#include "Application/app_printf.h"
#include "Drivers/Valve/ValveList.hpp"
#include "Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include/prc_intranet/const.hpp"

#include "main.h"
#include "stm32h7xx_hal.h"

using MessageId = prc_intranet::constants::MessageId;

namespace {

prc::PrcStore& Store() { return prc::PrcStore::get_instance(); }

BoardRole Role() { return Store().boardIdentityStore.get_role(); }

bool IsDpr(BoardRole role) { return role == BoardRole::DprLox || role == BoardRole::DprEth; }

const char* RoleName(BoardRole role) {
  switch (role) {
    case BoardRole::DprLox:    return "DPR_LOX";
    case BoardRole::DprEth:    return "DPR_ETH";
    case BoardRole::EngineBay: return "ENGINE";
    default:                   return "UNKNOWN";
  }
}

const char* OpenClosed(bool open) { return open ? "open" : "closed"; }

// Same write as prc_can.cpp's file local SetCmd (kept there, that file is
// not ours to change): the FSM picks the id up from intranetCmd on its next
// tick.
void SetCmd(MessageId id, uint8_t value = 0) {
  IntranetCmd cmd;
  cmd.id = static_cast<uint16_t>(id);
  cmd.value = value;
  Store().intranetCmdStore.set(cmd);
}

void RefuseRole(const char* command, const char* boards) {
  app_printf("[SHELL] %s refused on %s: %s only\r\n", command, RoleName(Role()), boards);
}

std::string DprStateName() {
  PrcState names;  // stateToString is a member; the constructor only sets MANUAL
  return names.stateToString(Store().stateStore.get());
}

// DPR manual valve result, straight from Prc_Fsm_Manual*'s return value.
void ReportDpr(const char* command, const char* value, bool applied) {
  if (applied) {
    app_printf("[SHELL] %s %s on %s\r\n", command, value, RoleName(Role()));
  } else {
    app_printf("[SHELL] %s %s refused on %s: FSM %s allows manual valves only in MANUAL, PRESSURIZE_ON, REGULATE\r\n",
               command, value, RoleName(Role()), DprStateName().c_str());
  }
}

// Engine manual valve or igniter result, from Prc_Engine_ManualSet*.
void ReportEngine(const char* command, const char* value, bool applied) {
  if (applied) {
    app_printf("[SHELL] %s %s on %s\r\n", command, value, RoleName(Role()));
  } else {
    app_printf("[SHELL] %s %s refused on %s: engine FSM %s, manual commands only in IDLE\r\n",
               command, value, RoleName(Role()),
               PrcEngineState::stateToString(Prc_Engine_Fsm_GetState()).c_str());
  }
}

// abort, passivate and reset: this board's own message id.
void SendOwnCmd(const char* command, MessageId lox, MessageId eth, MessageId engine) {
  const BoardRole role = Role();
  if (role == BoardRole::DprLox) SetCmd(lox);
  else if (role == BoardRole::DprEth) SetCmd(eth);
  else if (role == BoardRole::EngineBay) SetCmd(engine);
  else {
    RefuseRole(command, "DPR_LOX, DPR_ETH and ENGINE");
    return;
  }
  app_printf("[SHELL] %s sent on %s\r\n", command, RoleName(role));
}

const char* UnitFor(const char* name) {
  if (std::strncmp(name, "pressure", 8) == 0)          return "bar";
  if (std::strncmp(name, "temperature", 11) == 0)      return "C";
  if (std::strncmp(name, "integral_pressure", 17) == 0) return "bar.s";
  if (std::strncmp(name, "total_engine_impuse", 19) == 0) return "N.s";
  return "";
}

void PrintSensor(const char* name, double value) {
  app_printf("[SHELL]   %s = %.3f %s\r\n", name, value, UnitFor(name));
}

}  // namespace

namespace prc_commands {

bool IsProtectedMode() {
  const BoardRole role = Role();
  if (role == BoardRole::EngineBay) {
    return Prc_Engine_Fsm_GetState() != EngineState::Idle;
  }
  if (IsDpr(role)) {
    const State state = Store().stateStore.get();
    return !(state == State::MANUAL || state == State::PRESSURIZE_ON || state == State::REGULATE);
  }
  return true;
}

void OnState(void*) noexcept {
  const BoardRole role = Role();
  std::string fsm = "none";
  if (role == BoardRole::EngineBay) fsm = PrcEngineState::stateToString(Prc_Engine_Fsm_GetState());
  else if (IsDpr(role)) fsm = DprStateName();

  app_printf("[SHELL] state %s: fsm=%s protected=%s uptime=%lu ms\r\n",
             RoleName(role), fsm.c_str(), IsProtectedMode() ? "yes" : "no",
             static_cast<unsigned long>(HAL_GetTick()));

  for (uint8_t i = 0; i < static_cast<uint8_t>(ValveId::Count); i++) {
    const ValveId id = static_cast<ValveId>(i);
    const IValve* valve = Valve_Get(id);
    if (valve == nullptr) continue;
    if (id == ValveId::BallValve) {
      const prc::Valves& v = Store().valvesStore.get();
      const float pos = role == BoardRole::DprLox ? v.ball_valve_lox : v.ball_valve_fuel;
      app_printf("[SHELL]   %s = %.1f %%\r\n", valve->name(), pos);
    } else {
      app_printf("[SHELL]   %s = %s\r\n", valve->name(), OpenClosed(valve->is_open()));
    }
  }
  if (role == BoardRole::EngineBay) {
    const bool on = HAL_GPIO_ReadPin(Igniter_GPIO_Port, Igniter_Pin) == GPIO_PIN_SET;
    app_printf("[SHELL]   igniter = %s\r\n", on ? "on" : "off");
  }
}

void OnSensors(void*) noexcept {
  const BoardRole role = Role();
  app_printf("[SHELL] sensors %s:\r\n", RoleName(role));

#define X_raw_FIELD(type, field) PrintSensor(#field, s.get_##field());
#define X_proc_FIELD(type, field) PrintSensor(#field "_mean", s.get_##field##_mean());
#define X_FIELD(type, field) X_raw_FIELD(type, field) X_proc_FIELD(type, field)
  if (role == BoardRole::EngineBay) {
    const auto& s = Store().propSensorsStoreEngine;
    X_PRC_SENSORS_STORE_ENGINE()
  } else if (role == BoardRole::DprLox) {
    const auto& s = Store().propSensorsStoreLox;
    X_PRC_SENSORS_STORE_LOX()
  } else if (role == BoardRole::DprEth) {
    const auto& s = Store().propSensorsStoreEth;
    X_PRC_SENSORS_STORE_ETH()
  }
#undef X_raw_FIELD
#undef X_proc_FIELD
#undef X_FIELD
}

void OnValveVent(void*, bool open) noexcept {
  if (!IsDpr(Role())) { RefuseRole("valve vent", "DPR_LOX and DPR_ETH"); return; }
  ReportDpr("valve vent", OpenClosed(open), Prc_Fsm_ManualSetVent(open));
}

void OnValveSafety(void*, bool open) noexcept {
  if (!IsDpr(Role())) { RefuseRole("valve safety", "DPR_LOX and DPR_ETH"); return; }
  ReportDpr("valve safety", OpenClosed(open), Prc_Fsm_ManualSetSafety(open));
}

void OnValveCopvVent(void*, bool open) noexcept {
  if (!IsDpr(Role())) { RefuseRole("valve copv_vent", "DPR_LOX and DPR_ETH"); return; }
  ReportDpr("valve copv_vent", OpenClosed(open), Prc_Fsm_ManualVentCopv(open));
}

// The reply shows the position the driver applied: ServoBallValve clamps to
// 15 to 75 percent. Out of range or NaN requests are refused here with that
// reason, instead of reading as an FSM refusal.
void OnValveBall(void*, float percent_open) noexcept {
  if (!IsDpr(Role())) { RefuseRole("valve ball", "DPR_LOX and DPR_ETH"); return; }
  if (!(percent_open >= 0.0f && percent_open <= 100.0f)) {
    app_printf("[SHELL] valve ball refused on %s: position must be 0 to 100\r\n", RoleName(Role()));
    return;
  }
  char value[48];
  const bool applied = Prc_Fsm_ManualSetBallValve(percent_open);
  const ServoBallValve* ball = Valve_GetBallValve();
  if (applied && ball != nullptr && ball->position() != percent_open) {
    std::snprintf(value, sizeof(value), "%.1f%% (clamped from %.1f%%)", ball->position(), percent_open);
  } else {
    std::snprintf(value, sizeof(value), "%.1f%%", percent_open);
  }
  ReportDpr("valve ball", value, applied);
}

void OnValveMo(void*, bool open) noexcept {
  if (Role() != BoardRole::EngineBay) { RefuseRole("valve mo", "ENGINE"); return; }
  ReportEngine("valve mo", OpenClosed(open), Prc_Engine_ManualSetMo(open));
}

void OnValveMe(void*, bool open) noexcept {
  if (Role() != BoardRole::EngineBay) { RefuseRole("valve me", "ENGINE"); return; }
  ReportEngine("valve me", OpenClosed(open), Prc_Engine_ManualSetMe(open));
}

void OnIgniter(void*, bool on) noexcept {
  if (Role() != BoardRole::EngineBay) { RefuseRole("igniter", "ENGINE"); return; }
  ReportEngine("igniter", on ? "on" : "off", Prc_Engine_ManualSetIgniter(on));
}

void OnFsmPressurize(void*, bool on) noexcept {
  const BoardRole role = Role();
  if (!IsDpr(role)) { RefuseRole("fsm pressurize", "DPR_LOX and DPR_ETH"); return; }
  SetCmd(role == BoardRole::DprLox ? MessageId::dpr_lox_pressurize : MessageId::dpr_eth_pressurize, on ? 1 : 0);
  app_printf("[SHELL] fsm pressurize %s sent on %s\r\n", on ? "on" : "off", RoleName(role));
}

void OnFsmClearToIgnite(void*) noexcept {
  if (Role() != BoardRole::EngineBay) { RefuseRole("fsm clear_to_ignite", "ENGINE"); return; }
  SetCmd(MessageId::prc_clear_to_ignite);
  app_printf("[SHELL] fsm clear_to_ignite sent on %s\r\n", RoleName(Role()));
}

void OnFsmIgnite(void*) noexcept {
  if (Role() != BoardRole::EngineBay) { RefuseRole("fsm ignite", "ENGINE"); return; }
  SetCmd(MessageId::prc_ignite);
  app_printf("[SHELL] fsm ignite sent on %s\r\n", RoleName(Role()));
}

void OnFsmAbort(void*) noexcept {
  SendOwnCmd("fsm abort", MessageId::dpr_lox_abort, MessageId::dpr_eth_abort, MessageId::broadcast_abort);
}

void OnFsmPassivate(void*) noexcept {
  SendOwnCmd("fsm passivate", MessageId::dpr_lox_passivate, MessageId::dpr_eth_passivate, MessageId::prc_passivate);
}

void OnFsmReset(void*) noexcept {
  SendOwnCmd("fsm reset", MessageId::dpr_lox_reset, MessageId::dpr_eth_reset, MessageId::prc_reset);
}

// The confirmation goes out while the USB log is still on, so "logs usb off"
// still gets its reply.
void OnLogsUsb(void*, bool on) noexcept {
  if (on) app_printf_use_usb(true);
  app_printf("[SHELL] logs usb %s on %s\r\n", on ? "on" : "off", RoleName(Role()));
  if (!on) app_printf_use_usb(false);
}

void OnLogsCan(void*, bool on) noexcept {
  if (on) app_printf_use_can(true);
  app_printf("[SHELL] logs can %s on %s\r\n", on ? "on" : "off", RoleName(Role()));
  if (!on) app_printf_use_can(false);
}

void OnConfigPrintStatus(void*) noexcept {
  app_printf("[SHELL] config print status on %s\r\n", RoleName(Role()));
  config::internal::print_status();
}

// Committing swaps the live config the FSMs read every tick, so it is
// refused in protected mode (and on role Unknown, where IsProtectedMode is
// always true). The logs commands stay allowed on every role.
void OnConfigCommit(void*) noexcept {
  const BoardRole role = Role();
  if (role == BoardRole::Unknown) {
    app_printf("[SHELL] config commit refused on UNKNOWN: board role not latched\r\n");
    return;
  }
  if (IsProtectedMode()) {
    const std::string fsm = role == BoardRole::EngineBay
        ? PrcEngineState::stateToString(Prc_Engine_Fsm_GetState())
        : DprStateName();
    app_printf("[SHELL] config commit refused on %s: FSM %s is protected\r\n", RoleName(role), fsm.c_str());
    return;
  }
  config::internal::commit();
  app_printf("[SHELL] config commit on %s, crc 0x%08lX\r\n", RoleName(Role()),
             static_cast<unsigned long>(config::internal::crc_commited()));
}

void OnBadArgument(void*) noexcept {
  app_printf("[SHELL] command refused on %s: bool arguments are open, close, on or off\r\n", RoleName(Role()));
}

void FillDriver(prc_cli::driver& drv) {
  drv.on_bad_argument     = OnBadArgument;
  drv.config_commit       = OnConfigCommit;
  drv.config_print_status = OnConfigPrintStatus;
  drv.fsm_abort           = OnFsmAbort;
  drv.fsm_clear_to_ignite = OnFsmClearToIgnite;
  drv.fsm_ignite          = OnFsmIgnite;
  drv.fsm_passivate       = OnFsmPassivate;
  drv.fsm_pressurize      = OnFsmPressurize;
  drv.fsm_reset           = OnFsmReset;
  drv.igniter             = OnIgniter;
  drv.logs_can            = OnLogsCan;
  drv.logs_usb            = OnLogsUsb;
  drv.sensors_print       = OnSensors;
  drv.state_print         = OnState;
  drv.valve_ball          = OnValveBall;
  drv.valve_copv_vent     = OnValveCopvVent;
  drv.valve_me            = OnValveMe;
  drv.valve_mo            = OnValveMo;
  drv.valve_safety        = OnValveSafety;
  drv.valve_vent          = OnValveVent;
}

}  // namespace prc_commands

namespace {

prc_cli::driver g_driver;

void AppPrintfSink(const char* text) { app_printf("%s", text); }

}  // namespace

extern "C" void Prc_Shell_Init(void) {
  prc_commands::FillDriver(g_driver);
  prc_shell::Configure(&g_driver, AppPrintfSink);
}
