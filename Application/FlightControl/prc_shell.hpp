//    FILE: prc_shell.hpp
// PURPOSE: A tiny command shell over the PRC's USB VCP, the same shell the
//          FC has (see 2026_C_AV_FC's fc_shell.hpp), built from the CLI
//          submodule's prc_commands_generated.hpp (namespace prc_cli).
//
//          CDC_Receive_HS (USB interrupt) only copies bytes into a 256 byte
//          ring buffer through Prc_Shell_ProcessRxBytes. Prc_Shell_Tick
//          (main loop, top of main_tick) drains the ring, assembles lines of
//          up to 128 bytes across USB packets, and replays each complete
//          line character by character through prc_cli::push_char. Handlers
//          therefore run in the main loop, never in interrupt context.
//
//          No echo, no prompt: each handler prints one "[SHELL] ..." line
//          through app_printf (see prc_commands.hpp).
//
//          This file and prc_shell.cpp stay free of HAL includes so the line
//          assembler builds and runs in the host tests
//          (Application/Tests/test_prc_shell.cpp).

#ifndef APP_PRC_SHELL_H
#define APP_PRC_SHELL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Producer side, USB interrupt only. Copies raw bytes from one
// CDC_Receive_HS call into the ring buffer and returns; never parses. A
// packet that does not fit is dropped whole, and the line it belonged to is
// discarded by the consumer instead of running with a hole in it.
void Prc_Shell_ProcessRxBytes(const uint8_t *data, uint32_t length);

// Consumer side, main loop only. Drains the ring buffer and runs every
// complete line. Calls Prc_Shell_Init on its first run.
void Prc_Shell_Tick(void);

// Binds the PRC command handlers (prc_commands.cpp) and the app_printf
// reply sink. Defined in prc_commands.cpp, not prc_shell.cpp, so the host
// tests can bind a fake driver instead.
void Prc_Shell_Init(void);

#ifdef __cplusplus
}

#include "prc_commands_generated.hpp"

namespace prc_shell {

constexpr uint16_t kRingSize = 256;
constexpr uint32_t kLineMax  = 128;

// Where the shell's own messages go (overlong or damaged line notices).
// Command replies are printed by the handlers themselves.
using Sink = void (*)(const char *text);

// Installs the driver the lines are dispatched to and the sink for the
// shell's own messages, and resets the line assembler. Prc_Shell_Init calls
// it on the target; the host tests call it with a fake driver.
void Configure(const prc_cli::driver *drv, Sink sink);

}  // namespace prc_shell

#endif

#endif // APP_PRC_SHELL_H
