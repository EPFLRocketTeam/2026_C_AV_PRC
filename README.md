# 2026_C_AV_PRC

Propulsion computer firmware (STM32H743). One image serves the engine, LOX and ethanol boards; the role is read from the board straps at boot.

## Shell

The PRC has the same command shell as the flight computer, on its USB port (USB CDC, it shows up as a serial port).

Open the port with any serial terminal (PuTTY, screen, minicom, the CubeIDE terminal). The baud rate is ignored by CDC, so 115200 or anything else works. Each command is one line ending in CR, LF or CRLF, so the terminal default for Enter works; trailing spaces are dropped. There is no echo and no prompt, so turn on local echo in the terminal if you want to see what you type. Every command answers with one `[SHELL] ...` line naming the board role, or a refusal saying why. The first bytes received turn the USB log on, so replies show up without sending `logs usb on` first.

USB output never holds up the main loop. A log line goes out only while a host has the port configured. If the port stays busy for 2 ms (laptop unplugged, terminal not reading), that line and every later one are dropped until the host is seen again: a new enumeration, a byte received or a transfer completed. Unplugging a laptop therefore costs log output, never loop time.

Bool arguments are true only for `open` or `on`; anything else means closed or off.

| Command | Boards | What it does |
| --- | --- | --- |
| `state` | all | Role, FSM state, protected mode, uptime in ms, then every valve by name (and the igniter on the engine board). |
| `sensors` | all | This board's sensor values with units, one per line. |
| `valve vent open\|close` | DPR | Vent valve, through the DPR FSM's manual override. |
| `valve safety open\|close` | DPR | Safety valve, same override. |
| `valve copv_vent open\|close` | DPR | Vent, safety and ball valve together to vent the COPV. |
| `valve ball <percent>` | DPR | Ball valve position, 0 to 100. |
| `valve mo open\|close` | engine | Main oxidiser valve. |
| `valve me open\|close` | engine | Main ethanol valve. |
| `igniter on\|off` | engine | Igniter output. |
| `fsm pressurize on\|off` | DPR | Pressurize command, as if it came over CAN. |
| `fsm clear_to_ignite` | engine | Clear to ignite. |
| `fsm ignite` | engine | Ignite. |
| `fsm abort` | all | Abort (DPR abort on a DPR board, broadcast abort on the engine board). |
| `fsm passivate` | all | Passivate this board. |
| `fsm reset` | all | Reset this board's FSM. |
| `logs usb on\|off` | all | app_printf over USB. |
| `logs can on\|off` | all | app_printf forwarded over CAN. |
| `config print status` | all | Config buffer and committed CRCs. |
| `config commit` | all | Commit the config buffer. |

Manual valve commands are refused while the board is in a protected state: DPR boards accept them only in MANUAL, PRESSURIZE_ON and REGULATE (the FSM's own allow list), the engine board only in IDLE. A line longer than 128 bytes, or one that lost bytes to a full receive buffer, is dropped with a message instead of being run.

The command tree lives in the CLI submodule (`Drivers/CLI/2026_C_AV_CLI/prc_commands.yaml`); the parser `prc_commands_generated.*` is generated from it with `--namespace prc_cli`. The handlers are in `Application/FlightControl/prc_commands.cpp`, the line assembler in `prc_shell.cpp`.
