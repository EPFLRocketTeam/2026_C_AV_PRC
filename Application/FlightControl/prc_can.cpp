#include "Application/FlightControl/prc_can.hpp"

#include <cstdio>
#include <cstring>

#include "Application/Data/data.hpp"
#include "Application/FlightControl/uplink_cmd.hpp"
#include "prc_intranet/const.hpp"
#include "prc_intranet/dispatch.hpp"
#include "prc_intranet/transmit.hpp"

using namespace prc;
namespace pi = prc_intranet;

namespace {

void SetCmd(uint8_t id, uint8_t value = 0) {
  UplinkCmd cmd;
  cmd.id = id;
  cmd.value = value;
  PrcStore::get_instance().uplinkCmdStore.set(cmd);
}

BoardRole CurrentRole() {
  return PrcStore::get_instance().boardIdentityStore.get_role();
}

// Broadcast abort applies to every DPR node regardless of role (the outer
// role gate in Prc_Can_ProcessRxMessage below already excludes EngineBay/
// Unknown before this ever runs).
void OnBroadcastAbort(void*, pi::payload::safety_key key) noexcept {
  if (key.safety_key == pi::constants::SAFETY_KEY_BROADCAST_ABORT) {
    SetCmd(kCmdAbort);
  }
}

// DPR_ETH_* and DPR_LOX_* both get wired into the same driver (RX for this
// board's own node id is already the only traffic the hardware filter lets
// through, see Prc_Can_ConfigNodeFilter), but each callback still checks
// CurrentRole() as a second, software-level guard. This matches the
// original hand-written decode, which only ever compared can_id against
// this board's own role-specific ids.

void OnDprEthAbort(void*, pi::payload::safety_key key) noexcept {
  if (CurrentRole() != BoardRole::DprEth) return;
  if (key.safety_key == pi::constants::SAFETY_KEY_DPR_ETH_ABORT) SetCmd(kCmdAbort);
}
void OnDprEthPassivate(void*, pi::payload::safety_key key) noexcept {
  if (CurrentRole() != BoardRole::DprEth) return;
  if (key.safety_key == pi::constants::SAFETY_KEY_DPR_ETH_PASSIVATE) SetCmd(kCmdPassivate);
}
void OnDprEthPressurize(void*, pi::payload::on_off cmd) noexcept {
  if (CurrentRole() != BoardRole::DprEth) return;
  if (cmd.state == pi::constants::CMD_ON) SetCmd(kCmdPressurizeOn);
  else if (cmd.state == pi::constants::CMD_OFF) SetCmd(kCmdPressurizeOff);
}
void OnDprEthReset(void*, pi::payload::reset r) noexcept {
  if (CurrentRole() != BoardRole::DprEth) return;
  if (r.magic == pi::constants::RESET_MAGIC) SetCmd(kCmdReset);
}
// TODO: manual single-valve override, decoded here but not acted on yet.
// VALVE_SAFETY/VALVE_VENT/VALVE_BALLVALVE (prc_intranet/const.hpp) are
// still placeholder byte values; confirm before wiring this to
// Valve_Get(k_valve_safety)/Valve_Get(k_valve_vent) (see prc_state.cpp).
// Likely only meaningful in MANUAL, same as the engine PRC's CMD_VALVES
// override -- and per 2025_C_PR_DPR's precedent, should probably be gated
// to MANUAL/ABORT_ON_GROUND only, not open in every state. The ball valve
// is proportional (ServoBallValve::set_position), not open/closed, so
// VALVE_BALLVALVE has no meaningful action here yet.
void OnDprEthCmdValves(void*, pi::payload::cmd_valves) noexcept {
  if (CurrentRole() != BoardRole::DprEth) return;
}

void OnDprLoxAbort(void*, pi::payload::safety_key key) noexcept {
  if (CurrentRole() != BoardRole::DprLox) return;
  if (key.safety_key == pi::constants::SAFETY_KEY_DPR_LOX_ABORT) SetCmd(kCmdAbort);
}
void OnDprLoxPassivate(void*, pi::payload::safety_key key) noexcept {
  if (CurrentRole() != BoardRole::DprLox) return;
  if (key.safety_key == pi::constants::SAFETY_KEY_DPR_LOX_PASSIVATE) SetCmd(kCmdPassivate);
}
void OnDprLoxPressurize(void*, pi::payload::on_off cmd) noexcept {
  if (CurrentRole() != BoardRole::DprLox) return;
  if (cmd.state == pi::constants::CMD_ON) SetCmd(kCmdPressurizeOn);
  else if (cmd.state == pi::constants::CMD_OFF) SetCmd(kCmdPressurizeOff);
}
void OnDprLoxReset(void*, pi::payload::reset r) noexcept {
  if (CurrentRole() != BoardRole::DprLox) return;
  if (r.magic == pi::constants::RESET_MAGIC) SetCmd(kCmdReset);
}
void OnDprLoxCmdValves(void*, pi::payload::cmd_valves) noexcept {
  if (CurrentRole() != BoardRole::DprLox) return;
}

// HAL_FDCAN_AddMessageToTxFifoQ word-copies from this buffer regardless of
// dlc (see 2026_C_AV_FC's main.c TX test comment), so pad to the full
// word-aligned MAX_PAYLOAD_SIZE rather than passing `buffer` (only
// guaranteed readable for `dlc` bytes) straight through. driver_ptr carries
// the FDCAN_HandleTypeDef* set by the caller of the send_* functions
// below, the same way Prc_Can_ConfigNodeFilter takes it as a parameter
// rather than assuming a global.
void CbSend(void* driver_ptr, uint16_t can_id, const uint8_t* buffer, uint32_t dlc) noexcept {
  auto* hfdcan = static_cast<FDCAN_HandleTypeDef*>(driver_ptr);

  FDCAN_TxHeaderTypeDef txHeader;
  txHeader.Identifier          = can_id;
  txHeader.IdType              = FDCAN_STANDARD_ID;
  txHeader.TxFrameType         = FDCAN_DATA_FRAME;
  txHeader.DataLength          = dlc; // FDCAN_DLC_BYTES_0..8 == byte count for classic frames, see the RX comment above
  txHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
  txHeader.BitRateSwitch       = FDCAN_BRS_OFF;
  txHeader.FDFormat            = FDCAN_CLASSIC_CAN;
  txHeader.TxEventFifoControl  = FDCAN_NO_TX_EVENTS;
  txHeader.MessageMarker       = 0;

  uint8_t txData[pi::constants::MAX_PAYLOAD_SIZE] = {0};
  memcpy(txData, buffer, dlc);

  if (HAL_FDCAN_AddMessageToTxFifoQ(hfdcan, &txHeader, txData) != HAL_OK) {
    printf("[PRC CAN] TX failed, id=0x%X\r\n", can_id);
  }
}

pi::context& Ctx() {
  static pi::context ctx = [] {
    pi::prc_driver driver{};
    driver.send                  = CbSend;
    driver.on_broadcast_abort    = OnBroadcastAbort;
    driver.on_dpr_eth_abort      = OnDprEthAbort;
    driver.on_dpr_eth_passivate  = OnDprEthPassivate;
    driver.on_dpr_eth_pressurize = OnDprEthPressurize;
    driver.on_dpr_eth_reset      = OnDprEthReset;
    driver.on_dpr_eth_cmd_valves = OnDprEthCmdValves;
    driver.on_dpr_lox_abort      = OnDprLoxAbort;
    driver.on_dpr_lox_passivate  = OnDprLoxPassivate;
    driver.on_dpr_lox_pressurize = OnDprLoxPressurize;
    driver.on_dpr_lox_reset      = OnDprLoxReset;
    driver.on_dpr_lox_cmd_valves = OnDprLoxCmdValves;
    return pi::create_context(driver);
  }();
  return ctx;
}

} // namespace

void Prc_Can_ProcessRxMessage(uint32_t can_id, const uint8_t *data, uint32_t dlc) {
  const BoardRole role = CurrentRole();
  if (role != BoardRole::DprLox && role != BoardRole::DprEth) {
    // EngineBay runs a different FSM (not built here, see
    // prc_state.cpp's Prc_Fsm_Tick) and Unknown means role detection
    // hasn't latched yet. Either way, this decode doesn't apply.
    return;
  }

  pi::dispatch_frame(&Ctx(), static_cast<uint16_t>(can_id), data, dlc);
}

void Prc_Can_ConfigNodeFilter(FDCAN_HandleTypeDef *hfdcan) {
  const BoardRole role = CurrentRole();
  if (role != BoardRole::DprLox && role != BoardRole::DprEth) {
    // EngineBay/Unknown: this decode layer doesn't handle EngineBay's
    // messages, and Unknown means detection failed (see
    // PrcBoardId.cpp). Either way, no node to filter on.
    return;
  }

  const auto node = (role == BoardRole::DprLox) ? pi::can::Node::DprLox : pi::can::Node::DprEth;

  FDCAN_FilterTypeDef filter;
  filter.IdType       = FDCAN_STANDARD_ID;
  filter.FilterIndex  = 1; // index 0 is the static FC-broadcast filter, see main.c
  filter.FilterType   = FDCAN_FILTER_MASK;
  filter.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
  filter.FilterID1    = static_cast<uint32_t>(node) << 4;
  filter.FilterID2    = 0x0F0; // mask: only the node nibble must match

  if (HAL_FDCAN_ConfigFilter(hfdcan, &filter) != HAL_OK) {
    printf("[PRC CAN] node filter config failed, node=0x%X\r\n", static_cast<uint8_t>(node));
  }
}

void Prc_Can_SendTelemetry(FDCAN_HandleTypeDef *hfdcan) {
  const BoardRole role = CurrentRole();
  if (role != BoardRole::DprLox && role != BoardRole::DprEth) {
    return;
  }

  pi::context& ctx = Ctx();
  ctx.driver.driver_ptr = hfdcan;

  auto& sensors = PrcStore::get_instance().propSensorsStore;
  auto& valves  = PrcStore::get_instance().valvesStore;
  const bool is_lox = (role == BoardRole::DprLox);

  // p_xta/t_xta = this board's own propellant tank; p_nco/t_nco = the
  // shared COPV/N2 pressurant, same tank-vs-COPV split already confirmed
  // by prc_state.cpp's CurrentTankPressureBar()/CurrentCopvPressureBar().
  pi::payload::dpr_pressures pressures{};
  pressures.p_xta = static_cast<float>(is_lox ? sensors.get_LOX_pressure() : sensors.get_fuel_pressure());
  pressures.p_nco = static_cast<float>(sensors.get_N2_pressure());

  pi::payload::dpr_temps_1 temps{};
  temps.t_xta = static_cast<float>(is_lox ? sensors.get_LOX_temperature() : sensors.get_fuel_temperature());
  temps.t_nco = static_cast<float>(sensors.get_N2_temperature());

  // ValvesStore's valve_dpr_pressure_*/valve_dpr_vent_* are this board's
  // own Safety/Vent solenoids (see prc_state.cpp's SetSafety()/SetVent()).
  // The ball valve has no bit here, see const.hpp.
  const bool safety_open = is_lox ? valves.get_valve_dpr_pressure_lox() : valves.get_valve_dpr_pressure_fuel();
  const bool vent_open   = is_lox ? valves.get_valve_dpr_vent_lox()     : valves.get_valve_dpr_vent_fuel();

  pi::payload::dpr_state state{};
  state.fsm_state = static_cast<uint8_t>(PrcStore::get_instance().stateStore.get());
  state.valve_mask = (safety_open ? pi::constants::VALVE_MASK_BIT_SAFETY : 0)
                    | (vent_open  ? pi::constants::VALVE_MASK_BIT_VENT   : 0);

  if (is_lox) {
    pi::send_dpr_lox_state(&ctx, state);
    pi::send_dpr_lox_pressures(&ctx, pressures);
    pi::send_dpr_lox_temps_1(&ctx, temps);
  } else {
    pi::send_dpr_eth_state(&ctx, state);
    pi::send_dpr_eth_pressures(&ctx, pressures);
    pi::send_dpr_eth_temps_1(&ctx, temps);
  }
}
