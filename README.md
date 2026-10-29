# 2026_C_AV_PRC

Firehorn II propulsion computer firmware (STM32H743VI).

## Flashing over USB (DFU)

The PRC flashes over USB without an ST-Link, through the STM32H743 ROM bootloader. DFU is the ROM bootloader only: the application starts no DFU device of its own.

The board has one USB port that carries data, the mini USB connector J7 on OTG_FS (PA11/PA12). The application runs its serial console (CDC) there; it enumerates as "ERT Firehorn II PRC" with the chip's 96 bit unique id as its serial number, so each board keeps a stable port name. The ROM bootloader takes over the same port.

1. Enter the bootloader. The PRC has no software entry yet: nothing on the PRC calls `Bootloader_RequestDfu()` (`Application/System/bootloader.h`) until a shell or CAN command binds it, so today the only route is BOOT0: set the BOOT0 switch high, reset or power cycle the board, and set BOOT0 back low once flashed.
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

Operational rule: never enter DFU on a loaded rocket. In the ROM bootloader the PRC drives no valve and answers no CAN command, and the bootloader has no timeout: it stays there until a reset or a power cycle.

What to expect:

* The ROM bootloader only runs at reset. `Bootloader_RequestDfu()` stores a magic word in RTC backup register 0 and resets; the first statement of `main()` sees it and jumps to the bootloader before the HAL starts (`Application/System/bootloader.cpp`).
* The word is cleared before the jump, so a power cycle or a reset out of the bootloader never enters DFU again.
* After flashing, `:leave` (or `--start`) exits the bootloader and the board reboots into the new image. With the BOOT0 fallback, every reset enters the bootloader until BOOT0 is back low.

Bench checklist, once a command binds `Bootloader_RequestDfu()` (the backup register write behind it is not yet confirmed on hardware):

1. Run that command.
2. Expect `0483:df11` on J7 (`dfu-util -l`).
3. If the board just reboots into the application instead, select LSI as the RTC clock source in the `.ioc` (RCC, RTC clock mux), regenerate, and try again.

What the ROM bootloader needs (ST AN2606 Rev 61, section 52 "STM32H74xxx/75xxx devices bootloader"):

* Entry point: "The bootloader start address is 0x1FF09800" (section 52.1, Table 113). The jump loads the initial stack pointer from 0x1FF09800 and the reset handler from 0x1FF09804.
* USB: DFU runs on "USB FS configured in forced device mode" on PA11 (DM) and PA12 (DP), the J7 pair, and "No external pull-up resistor is required" (Table 113).
* Clock: the bootloader runs at 64 MHz on HSI and uses the CRS to clock USB from HSI48 (Table 113), so no crystal is involved.
* Supply: "DFU mode does not support USBREGEN mode. If STM32 is powered by 1.8 V source, it is not possible to use the BL DFU unless 3.3 V is provided" (Table 113).
* Version: the bootloader ID byte at 0x1FF1E7FE is 0x91 for V9.1 (Table 177); `Bootloader_RomId()` returns it for whichever command binds DFU entry later. A part reading 0x90 has V9.0, whose known limitations (Table 114) are a jump issue unless the application stack pointer is below RAM end minus 16 bytes (the jump here hands over the bootloader's own initial stack pointer, which avoids it) and an extra reset needed after a power cycle before the bootloader answers (ST's workaround is a pull-up on PA11). V9.1 fixes both.
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
