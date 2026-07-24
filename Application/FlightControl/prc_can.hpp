//    FILE: prc_can.hpp
// PURPOSE: Decodes received FDCAN messages (dictionary defined in the
//          prc_intranet submodule's message_list.hpp) addressed to this
//          board's DPR node into UplinkCmd, consumed by PrcState::fromXxx()
//          (see uplink_cmd.hpp). Called from main.c's RX poll loop via the
//          extern "C" shim below.

#ifndef APP_PRC_CAN_H
#define APP_PRC_CAN_H

#include <stdint.h>

#include "stm32h7xx_hal.h"

// Comment this out to fully disable CAN log forwarding (Prc_Log_Forward
// becomes a no-op at the call site in main.c's _write()). Local VCP
// output is unaffected either way.
#define ENABLE_LOG

#ifdef __cplusplus
extern "C" {
#endif

// can_id: 11-bit standard CAN arbitration ID, as read from
// FDCAN_RxHeaderTypeDef::Identifier. data: up to 8 bytes, dlc: number of
// valid bytes in data (from FDCAN_RxHeaderTypeDef::DataLength decoded to a
// byte count).
//
// Messages not addressed to this board's role (BoardIdentityStore's latched
// BoardRole, DprLox/DprEth, see data.hpp) are ignored, as are EMERGENCY
// messages whose safety key doesn't match, and RESET messages whose magic
// doesn't match. Silent ignore, not an error: the CAN bus carries messages
// for other nodes too, this board just filters to its own.
void Prc_Can_ProcessRxMessage(uint32_t can_id, const uint8_t *data, uint32_t dlc);

// Configures FDCAN filter index 1 (see main.c's MX_FDCAN1_Init(), which
// leaves this index deliberately unset; filter index 0 there is the
// static FC-broadcast filter) to accept this board's own node ID (derived
// from BoardIdentityStore's latched BoardRole, DprLox/DprEth). Call once,
// after Prc_Fsm_Init() has run (role must already be latched). No-op for
// EngineBay/Unknown: this decode layer only handles the DPR roles.
void Prc_Can_ConfigNodeFilter(FDCAN_HandleTypeDef *hfdcan);

// Sends this board's periodic DPR telemetry over CAN: dpr_state (FSM state
// + Safety/Vent valve_mask, from StateStore/ValvesStore) and tank + COPV/N2
// pressures/temperatures (from PropSensorsStore). Call periodically, e.g.
// from the main loop's throttled slow path. No-op for EngineBay/Unknown,
// same gating as Prc_Can_ProcessRxMessage. The ball valve's proportional
// position has no representation in valve_mask (see const.hpp).
void Prc_Can_SendTelemetry(FDCAN_HandleTypeDef *hfdcan);

// Forwards `length` raw bytes from a local printf/_write() call over CAN
// as log_chunk frames (see the log_aggregator submodule for the actual
// chunking), tagged with this board's own role. No-op for EngineBay/
// Unknown, same gating as Prc_Can_ProcessRxMessage -- no log_chunk id is
// wired for EngineBay here. Call from _write() so every printf both
// still reaches the local VCP (unchanged) and gets relayed to FC.
void Prc_Log_Forward(FDCAN_HandleTypeDef *hfdcan, const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif // APP_PRC_CAN_H
