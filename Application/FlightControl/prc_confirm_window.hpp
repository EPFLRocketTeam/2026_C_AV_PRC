//    FILE: prc_confirm_window.hpp
// PURPOSE: Two step confirmation for the shell's system dfu and system
//          reboot (prc_commands.cpp). A copy of the FC's
//          fc_shell::ConfirmWindow (2026_C_AV_FC, fc_shell_guard.hpp) so
//          both boards behave alike; HAL free for the host tests.
//
//          Either command only arms a 10 s window and says so; `system
//          confirm` inside the window acts on whatever was armed. The second
//          step is a different command, so a reflex double entry or a
//          repeated line only re-arms. Arming again (either command)
//          restarts the window for that command; a refusal disarms;
//          confirming consumes the window. Times are milliseconds from any
//          free running clock (HAL_GetTick on the board); wrap around is
//          handled.

#ifndef APP_PRC_CONFIRM_WINDOW_H
#define APP_PRC_CONFIRM_WINDOW_H

#include <cstdint>

namespace prc_commands {

class ConfirmWindow {
 public:
  enum class Command : uint8_t { kNone, kDfu, kReboot };
  static constexpr uint32_t kWindowMs = 10000;

  void Arm(Command command, uint32_t now_ms) {
    armed_ = command;
    armed_at_ms_ = now_ms;
  }

  // Returns the armed command and disarms, or kNone when nothing is armed
  // or the window ran out.
  Command Confirm(uint32_t now_ms) {
    const Command armed = armed_;
    armed_ = Command::kNone;
    if (armed == Command::kNone || now_ms - armed_at_ms_ >= kWindowMs) return Command::kNone;
    return armed;
  }

  void Disarm() { armed_ = Command::kNone; }

 private:
  Command armed_ = Command::kNone;
  uint32_t armed_at_ms_ = 0;
};

}  // namespace prc_commands

#endif  // APP_PRC_CONFIRM_WINDOW_H
