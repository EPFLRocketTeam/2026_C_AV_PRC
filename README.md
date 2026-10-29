# 2026_C_AV_PRC

Firehorn II propulsion computer firmware (STM32H743VI).

## Flashing over USB (DFU)

The PRC flashes over USB without an ST-Link, through the STM32H743 ROM bootloader. DFU is the ROM bootloader only: the application starts no DFU device of its own.

The board has one USB port that carries data, the mini USB connector J7 on OTG_FS (PA11/PA12). The application runs its serial console (CDC) there; it enumerates as "ERT Firehorn II PRC" with the chip's 96 bit unique id as its serial number, so each board keeps a stable port name. The ROM bootloader takes over the same port.

1. Enter the bootloader. Once the PRC shell binds it, type `system dfu` on the serial port (`Bootloader_RequestDfu()` in `Application/System/bootloader.h` is the call behind it). Fallback, and the only route until then: set the BOOT0 switch high and reset or power cycle the board, then set BOOT0 back low once flashed.
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

What to expect:

* The ROM bootloader only runs at reset. `Bootloader_RequestDfu()` stores a magic word in RTC backup register 0 and resets; the first statement of `main()` sees it and jumps to the bootloader before the HAL starts (`Application/System/bootloader.cpp`).
* The word is cleared before the jump, so a power cycle or a reset out of the bootloader never enters DFU again.
* After flashing, `:leave` (or `--start`) exits the bootloader and the board reboots into the new image. With the BOOT0 fallback, every reset enters the bootloader until BOOT0 is back low.

CubeMX note: `USB_DEVICE/App/usb_device.c` starts the CDC on the OTG_FS core and no DFU class, which the `.ioc` does not describe yet. Regenerating code from the current `.ioc` puts the CDC back on OTG_HS (PB14/PB15, which reach no connector on 2026_C_AV_PC_V1) and the DFU stub back on OTG_FS; update the `.ioc` first.
