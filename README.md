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
| `system dfu` | all | Arms a reboot into the ROM DFU bootloader for 10 s and prints the FSM state and the ROM bootloader ID. DPR boards only in MANUAL, the engine board only in IDLE, never before the role is latched. |
| `system reboot` | all | Arms a plain reboot for 10 s, same states. |
| `system confirm` | all | Within 10 s of `system dfu` or `system reboot`, checks the state again and does the armed reset. Typing `system dfu` twice only re-arms. |

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

## Flashing over USB (DFU)

The PRC flashes over USB without an ST-Link, through the STM32H743 ROM bootloader. DFU is the ROM bootloader only: the application starts no DFU device of its own.

The board has one USB port that carries data, the mini USB connector J7 on OTG_FS (PA11/PA12). The application runs its serial console (CDC) there; it enumerates as "ERT Firehorn II PRC" with the chip's 96 bit unique id as its serial number, so each board keeps a stable port name. The ROM bootloader takes over the same port.

1. Enter the bootloader. On the USB port, type `system dfu`, then `system confirm` within 10 s (see Shell for the states it accepts); `system dfu` alone only arms and prints the FSM state and the ROM bootloader ID. Fallback: set the BOOT0 switch high, reset or power cycle the board, and set BOOT0 back low once flashed.
2. Keep the USB port plugged. `dfu-util -l` lists the bootloader as `0483:df11`.
3. Flash a raw binary (`arm-none-eabi-objcopy -O binary firmware.elf firmware.bin` turns the CubeIDE output into one):

   ```bash
   dfu-util -a 0 -s 0x08000000:leave -D firmware.bin
   ```

   The STM32CubeProgrammer equivalent:

   ```bash
   STM32_Programmer_CLI -c port=USB1 -w firmware.bin 0x08000000 -v --start 0x08000000
   ```

   On Windows, dfu-util needs the WinUSB driver bound to the DFU device (Zadig); STM32CubeProgrammer ships ST's own driver.

Operational rule: never type `system confirm` after `system dfu` or `system reboot` on a loaded rocket, and never use the BOOT0 fallback there. In the ROM bootloader the PRC drives no valve and answers no CAN command, and the bootloader has no timeout: it stays there until a reset or a power cycle.

What to expect:

* The ROM bootloader only runs at reset. Confirming `system dfu` calls `Bootloader_RequestDfu()`, which stores a magic word in RTC backup register 0 and resets; the first statement of `main()` sees it and jumps to the bootloader before the HAL starts (`Application/System/bootloader.cpp`).
* The word is cleared before the jump, so a power cycle or a reset out of the bootloader never enters DFU again.
* After flashing, `:leave` (or `--start`) exits the bootloader and the board reboots into the new image. With the BOOT0 fallback, every reset enters the bootloader until BOOT0 is back low.

Bench checklist, once per board revision (the backup register write behind `system dfu` is not yet confirmed on hardware):

1. Run `system dfu`, then `system confirm` within 10 s, with the board in MANUAL (DPR) or IDLE (engine).
2. Expect `0483:df11` on J7 (`dfu-util -l`).
3. If the board just reboots into the application instead, select LSI as the RTC clock source in the `.ioc` (RCC, RTC clock mux), regenerate, and try again.

What the ROM bootloader needs (ST AN2606 Rev 61, section 52 "STM32H74xxx/75xxx devices bootloader"):

* Entry point: "The bootloader start address is 0x1FF09800" (section 52.1, Table 113). The jump loads the initial stack pointer from 0x1FF09800 and the reset handler from 0x1FF09804.
* USB: DFU runs on "USB FS configured in forced device mode" on PA11 (DM) and PA12 (DP), the J7 pair, and "No external pull-up resistor is required" (Table 113).
* Clock: the bootloader runs at 64 MHz on HSI and uses the CRS to clock USB from HSI48 (Table 113), so no crystal is involved.
* Supply: "DFU mode does not support USBREGEN mode. If STM32 is powered by 1.8 V source, it is not possible to use the BL DFU unless 3.3 V is provided" (Table 113).
* Version: the bootloader ID byte at 0x1FF1E7FE is 0x91 for V9.1 (Table 177); the arming `system dfu` prints it. A part reading 0x90 has V9.0, whose known limitations (Table 114) are a jump issue unless the application stack pointer is below RAM end minus 16 bytes (the jump here hands over the bootloader's own initial stack pointer, which avoids it) and an extra reset needed after a power cycle before the bootloader answers (ST's workaround is a pull-up on PA11). V9.1 fixes both.
* After its own reset the bootloader disables every interrupt source and peripheral clock and initialises its clock, GPIOs, IWDG and SysTick itself (Figure 64), so it starts cleanly from the reset state the jump hands over. It refreshes the IWDG it configures (Table 113).

## CubeMX regeneration

Kept by a regeneration, because they sit inside USER CODE blocks: the `Bootloader_CheckAndJump()` call and its include in `Core/Src/main.c`, and the product string and UID serial number in `USB_DEVICE/App/usbd_desc.c` (an `#undef` of `USBD_PRODUCT_STRING_HS` and a serial descriptor that returns early, both written against the HS descriptor set the CDC uses).

Reverted by a regeneration from the current `.ioc`: `USB_DEVICE/App/usb_device.c`. The CDC goes back to OTG_HS (PB14/PB15, which reach no connector on 2026_C_AV_PC_V1, so the board stops showing up as a serial port), and the stub DFU device comes back on OTG_FS, advertising the ROM bootloader's own `0483:df11` on J7. Also check that the `excluding="App/usbd_dfu_if.c"` entry on the `USB_DEVICE` source folder in `.cproject` is still there.

To make the `.ioc` match (`2026_C_AV_PRC.ioc`), move the CDC to OTG_FS and drop OTG_HS:

* `USB_DEVICE.CLASS_NAME_FS=DFU` becomes CDC and `USB_DEVICE.CLASS_NAME_HS=CDC` goes (lines 446 and 447), with `USB_DEVICE.IPParameters`, `USB_DEVICE.VirtualMode-CDC_HS`, `VirtualMode-DFU_FS`, `VirtualModeFS` and `VirtualModeHS` (lines 448 to 452) to match
* PA11/PA12 stay `USB_OTG_FS_DM`/`USB_OTG_FS_DP` in `Device_Only` (lines 181 to 184)
* PB14/PB15 `USB_OTG_HS` (lines 205 to 208), `Mcu.Pin16`/`Mcu.Pin17`, `Mcu.IP14=USB_OTG_HS` and the `USB_OTG_HS.*` lines (455, 456) go
* `Mcu.Pin43=VP_USB_DEVICE_VS_USB_DEVICE_DFU_FS` and `Mcu.Pin44=VP_USB_DEVICE_VS_USB_DEVICE_CDC_HS` with their `.Mode`/`.Signal` lines (461 to 464) become one `VP_USB_DEVICE_VS_USB_DEVICE_CDC_FS`

Once the `.ioc` puts the CDC on OTG_FS, CubeMX generates `usbd_cdc_if.c` and `usbd_desc.c` with FS names: `CDC_Transmit_FS`, `CDC_Receive_FS`, `USBD_Interface_fops_FS`, the `hUsbDeviceFS` handle and the `FS_Desc` descriptor set. Then rename the caller `CDC_Transmit_HS` in `_write` (`Core/Src/main.c`), move the code in the `CDC_Receive_HS` USER CODE block to `CDC_Receive_FS`, and move the product string and serial overrides from the HS names (`USBD_PRODUCT_STRING_HS`, `USBD_HS_SerialStrDescriptor`) to the FS ones. The product string can also be set on the USB_DEVICE middleware page; the 96 bit serial has no CubeMX parameter.
