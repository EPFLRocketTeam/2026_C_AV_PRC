# 2026_C_AV_PRC

Propulsion computer firmware (STM32H743). One image serves the engine, LOX and ethanol boards; the role is read from the board straps at boot.

## Shell

The PRC has the same command shell as the flight computer, on its USB port (USB CDC, it shows up as a serial port).

Open the port with any serial terminal (PuTTY, screen, minicom, the CubeIDE terminal). The baud rate is ignored by CDC, so 115200 or anything else works. Each command is one line ending in CR, LF or CRLF, so the terminal default for Enter works; trailing spaces are dropped. There is no echo and no prompt, so turn on local echo in the terminal if you want to see what you type. Every command answers with one `[SHELL] ...` line naming the board role, or a refusal saying why. The first bytes received turn the USB log on, so replies show up without sending `logs usb on` first.

USB output never holds up the main loop. A log line goes out only while a host has the port configured. If the port stays busy for 2 ms (laptop unplugged, terminal not reading), that line and every later one are dropped until the host is seen again: a new enumeration, a byte received or a transfer completed. Unplugging a laptop therefore costs log output, never loop time.

Bool arguments take `open` or `on` for true and `close` or `off` for false; any other word runs nothing and gets a refusal. A line that matches no command (an unknown word, a leading space, stray characters, a command cut short) runs nothing and answers `[SHELL] unknown command`; an empty line gets no answer.

| Command | Boards | What it does |
| --- | --- | --- |
| `state` | all | Role, FSM state, protected mode, uptime in ms, then every valve by name (and the igniter on the engine board). |
| `sensors` | all | This board's sensor values with units, one per line. |
| `valve vent open\|close` | DPR | Vent valve, through the DPR FSM's manual override. |
| `valve safety open\|close` | DPR | Safety valve, same override. |
| `valve copv_vent open\|close` | DPR | Vent, safety and ball valve together to vent the COPV. |
| `valve ball <percent>` | DPR | Ball valve position, 0 to 100. The driver clamps it to 15 to 75 and the reply shows the position applied. |
| `valve mo open\|close` | engine | Main oxidiser valve. |
| `valve me open\|close` | engine | Main ethanol valve. |
| `igniter on\|off` | engine | Igniter output. |
| `fsm pressurize on\|off` | DPR | This board's pressurize on or off command. |
| `fsm clear_to_ignite` | engine | Clear to ignite. |
| `fsm ignite` | engine | Ignite. |
| `fsm abort` | all | Abort this board's own FSM only. |
| `fsm passivate` | all | Passivate this board's own FSM only. |
| `fsm reset` | all | Reset this board's own FSM only. |
| `logs usb on\|off` | all | app_printf over USB. |
| `logs can on\|off` | all | app_printf forwarded over CAN. |
| `config print status` | all | Config buffer and committed CRCs. |
| `config commit` | all | Commit the config buffer. DPR boards only in MANUAL, the engine board only in IDLE, never before the role is latched. |

The `fsm` commands never go on the CAN bus. Each one writes the same command id into this board's own command slot that the matching CAN message from the FC would write, and this board's FSM acts on it on its next tick, only from the states listed here; in any other state it is ignored, while the reply still says "sent". The vehicle abort stays the FC's `prc abort` and DPR broadcast abort.

| Command | DPR board (LOX or ETH) | Engine board |
| --- | --- | --- |
| `fsm pressurize on` | MANUAL to PRESSURIZE_ON | refused |
| `fsm pressurize off` | REGULATE to PRESSURIZE_OFF | refused |
| `fsm clear_to_ignite` | refused | IDLE to CLEAR_TO_IGNITE |
| `fsm ignite` | refused | CLEAR_TO_IGNITE to IGNITION_PRECHILL, which starts the ignition sequence and its preburn messages to the DPR boards |
| `fsm abort` | writes this board's DPR abort id: PRESSURIZE_ON or REGULATE to ABORT_ON_GROUND, PRESSURIZE_OFF to ABORT_IN_FLIGHT | writes broadcast_abort into the local slot only, the DPR boards do not see it: CLEAR_TO_IGNITE and the ignition states to ABORT_ON_GROUND, BURN, BURN_STOP_ME, BURN_STOP_MO and WAIT_FOR_PASSIVATION to ABORT_IN_FLIGHT; ignored in IDLE |
| `fsm passivate` | PRESSURIZE_OFF to DEPRESSURIZE_ON, unless coldflow mode | WAIT_FOR_PASSIVATION to PASSIVATION_SEPARATION_DELAY, unless coldflow mode |
| `fsm reset` | DEPRESSURIZE_OFF and ABORT_ON_GROUND to MANUAL, PRESSURIZE_OFF to MANUAL in coldflow mode | DEPRESSURIZE_CLOSE, ABORT_ON_GROUND_OXYDANT and ABORT_ON_GROUND_ETHANOL to IDLE, WAIT_FOR_PASSIVATION to IDLE in coldflow mode |

Manual valve commands are refused while the board is in a protected state: DPR boards accept them only in MANUAL, PRESSURIZE_ON and REGULATE (the FSM's own allow list), the engine board only in IDLE. A line longer than 128 bytes, or one that lost bytes to a full receive buffer, is dropped with a message instead of being run.

Known host quirk: a NUL byte marks the line it arrives in as damaged, the same marker the shell uses for lost bytes, so that line is dropped with the overflow message. A terminal in telnet mode that sends CR NUL on Enter therefore loses every line after the first. Use a raw serial terminal, or set the terminal to send CR, LF or CRLF only.

The command tree lives in the CLI submodule (`Drivers/CLI/2026_C_AV_CLI/prc_commands.yaml`); the parser `prc_commands_generated.*` is generated from it with `--namespace prc_cli`. The handlers are in `Application/FlightControl/prc_commands.cpp`, the line assembler in `prc_shell.cpp`.
