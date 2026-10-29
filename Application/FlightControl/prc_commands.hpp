//    FILE: prc_commands.hpp
// PURPOSE: The command handlers behind the PRC's USB shell (see
//          prc_shell.hpp), exposed so other command sources can trigger the
//          exact same actions. Signatures match the generated trie's
//          prc_cli::driver callbacks (CLI submodule,
//          prc_commands_generated.hpp), so they work as plain functions or
//          as function pointers. The leading void* is the trie's handler
//          context and is unused here, so calling a handler directly with
//          nullptr is fine, e.g. prc_commands::OnFsmAbort(nullptr).
//
//          One image serves all three boards, so every handler reads the
//          latched board role and prints a "[SHELL] ..." refusal naming the
//          role when the command is not for this board. Every handler prints
//          exactly one "[SHELL] ..." confirmation or refusal line (state and
//          sensors add detail lines after it). Handlers run in the main loop
//          (Prc_Shell_Tick), not in interrupt context.

#ifndef APP_PRC_COMMANDS_H
#define APP_PRC_COMMANDS_H

#include "prc_commands_generated.hpp"

#include <cstddef>  // data.hpp uses offsetof without including it

#include "Application/Data/data.hpp"
#include "Application/Data/engine_fsm.hpp"
#include "Application/FlightControl/prc_confirm_window.hpp"

namespace prc_commands {

// Window for config commit, which swaps the live config the FSMs read every
// tick: DPR boards only in MANUAL (REGULATE reads the target pressure and
// the safety thresholds from the config every tick), the engine board only
// in Idle, never before the role is latched. Pure, so the host tests can
// check it without the HAL.
inline bool ConfigCommitAllowed(prc::BoardRole role, prc::State dpr_state, prc::EngineState engine_state) {
  switch (role) {
    case prc::BoardRole::DprLox:
    case prc::BoardRole::DprEth:    return dpr_state == prc::State::MANUAL;
    case prc::BoardRole::EngineBay: return engine_state == prc::EngineState::Idle;
    default:                        return false;
  }
}

// Window for system dfu and system reboot, which reset the board: DPR
// boards only in MANUAL, the engine board only in Idle, never before the
// role is latched. A reset drops whatever the FSM is doing and DFU then
// stops every valve command until NRST or a power cycle. Pure, for the
// host tests.
inline bool ResetAllowed(prc::BoardRole role, prc::State dpr_state, prc::EngineState engine_state) {
  switch (role) {
    case prc::BoardRole::DprLox:
    case prc::BoardRole::DprEth:    return dpr_state == prc::State::MANUAL;
    case prc::BoardRole::EngineBay: return engine_state == prc::EngineState::Idle;
    default:                        return false;
  }
}

// Binds every handler below into `drv`.
void FillDriver(prc_cli::driver& drv);

// True when manual valve and igniter commands are refused on this board:
//   - DPR boards: the FSM is outside MANUAL, PRESSURIZE_ON and REGULATE.
//     This mirrors the allow list inside Prc_Fsm_Manual* (prc_state.cpp),
//     which stays the real gate: the DPR valve handlers call those functions
//     and report their return value, they never check this first.
//   - Engine board: the engine FSM is not Idle. The real gate is inside
//     Prc_Engine_ManualSet* (engine_state.cpp).
//   - Role not latched yet: always true.
bool IsProtectedMode();

// -- Read only ---------------------------------------------------------------
// Role, FSM state name, protected mode, uptime, then one line per valve.
void OnState(void*) noexcept;
// This board's own sensor store, one line per value with its unit.
void OnSensors(void*) noexcept;

// -- DPR manual valves (DPR boards only, through Prc_Fsm_Manual*) -------------
void OnValveVent(void*, bool open) noexcept;
void OnValveSafety(void*, bool open) noexcept;
void OnValveCopvVent(void*, bool open) noexcept;
void OnValveBall(void*, float percent_open) noexcept;

// -- Engine manual valves and igniter (engine board only, Idle only) ----------
void OnValveMo(void*, bool open) noexcept;
void OnValveMe(void*, bool open) noexcept;
void OnIgniter(void*, bool on) noexcept;

// -- FSM commands ---------------------------------------------------------------
// Written into intranetCmdStore exactly as prc_can.cpp's CAN handlers do, so
// the FSM consumes them on its next tick. pressurize is DPR only,
// clear_to_ignite and ignite are engine only; abort, passivate and reset map
// to this board's own message id (engine abort is broadcast_abort, the id
// the engine FSM listens to).
void OnFsmPressurize(void*, bool on) noexcept;
void OnFsmClearToIgnite(void*) noexcept;
void OnFsmIgnite(void*) noexcept;
void OnFsmAbort(void*) noexcept;
void OnFsmPassivate(void*) noexcept;
void OnFsmReset(void*) noexcept;

// -- Logs and config -------------------------------------------------------------
void OnLogsUsb(void*, bool on) noexcept;
void OnLogsCan(void*, bool on) noexcept;
void OnConfigPrintStatus(void*) noexcept;
// Refused outside ConfigCommitAllowed's window, with the reason printed.
void OnConfigCommit(void*) noexcept;

// -- System (two step, see prc_confirm_window.hpp) --------------------------------
// system dfu and system reboot only arm a 10 s window, refused outside
// ResetAllowed, and print the FSM state and the ROM bootloader id.
// system confirm inside the window does the armed reset: the ROM DFU
// bootloader (Application/System/bootloader.h) or a plain reboot.
void OnSystemDfu(void*) noexcept;
void OnSystemReboot(void*) noexcept;
void OnSystemConfirm(void*) noexcept;

// Bound as the trie's on_bad_argument: a bool argument other than open,
// close, on or off runs nothing and gets this refusal instead.
void OnBadArgument(void*) noexcept;

// Bound as the trie's on_unknown_command: a non-empty line that matches no
// command (garbage, a leading space, a partial command).
void OnUnknownCommand(void*) noexcept;

}  // namespace prc_commands

#endif  // APP_PRC_COMMANDS_H
