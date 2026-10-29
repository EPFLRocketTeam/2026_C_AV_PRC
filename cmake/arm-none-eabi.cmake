# arm-none-eabi toolchain for the PRC firmware (STM32H743VITx, Cortex-M7).
#
# What was read from .cproject, Debug configuration (line numbers on main 9b2dc24):
#   MCU            STM32H743VITx, fpu fpv5-d16, float abi hard            (:20,23,24)
#   Source folders Application, Core, Drivers, Middlewares, Modules,
#                  USB_DEVICE; ThirdParty is not a source folder           (:144-149)
#   Exclusions     Drivers: Plume/2026_C_AV_PLUME/src/plume/cli,
#                  Plume/2026_C_AV_PLUME/src/plume/drivers,
#                  Plume/2026_C_AV_PLUME/tests,
#                  STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_msp_template.c,
#                  STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_wakeup_template.c,
#                  STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_alarm_template.c,
#                  CMSIS/DSP, CMSIS/Device/ST/STM32H7xx/Source, CMSIS/Core/Template,
#                  CMSIS/Core_A/Source, CMSIS/NN, CMSIS/RTOS2              (:146)
#                  Middlewares: ST/STM32_USB_Device_Library/Core/Src/usbd_desc_template.c,
#                  ST/STM32_USB_Device_Library/Core/Src/usbd_conf_template.c (:147)
#                  Modules: Sensors/tests                                  (:148)
#   C defines      DEBUG USE_PWR_LDO_SUPPLY USE_HAL_DRIVER STM32H743xx     (:47-52)
#   C++ defines    same four                                               (:106-111)
#   ASM defines    DEBUG                                                   (:33-35)
#   ASM includes   Drivers/PT1000, Drivers/FDC1004, ThirdParty/SignalUtils/include,
#                  Drivers/Plume/2026_C_AV_PLUME/include                   (:36-41)
#   C includes     USB_DEVICE/App, Application/Config/2026_C_AV_FLIGHT_PARAMS(+/2026_C_AV_CONFMAN),
#                  USB_DEVICE/Target, Core/Inc, HAL Inc(+Legacy), USB Core/CDC/DFU Inc,
#                  CMSIS Device Include, CMSIS Include, Drivers/PT1000, Drivers/FDC1004,
#                  root, Drivers/Valve, Drivers/KULITE_CTL190, Drivers/LMT85,
#                  ThirdParty/SignalUtils/include, Modules,
#                  Drivers/Plume/2026_C_AV_PLUME/include                   (:53-75)
#   C++ includes   every C path, plus Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include,
#                  Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include (:81-105)
#   Optimisation   empty, so the compiler default -O0; -g3 everywhere      (:32,45-46,79-80)
#   C standard     not set (compiler default)
#   C++ standard   gnu++17                                                 (:112)
#   Runtime        not set: CubeIDE default, reduced C (--specs=nano.specs)
#   System calls   minimal implementation (--specs=nosys.specs)            (:123)
#   Startup        Core/Startup/startup_stm32h743vitx.s (inside Core)
#   Linker script  STM32H743VITX_FLASH.ld                                  (:116,124)
#   Link flags     -u _printf_float                                        (:126)
#   Convert binary on                                                      (:28)
#
# CubeIDE adds these at build time without recording them in .cproject; they are
# its defaults and are applied in cmake/firmware.cmake:
#   -ffunction-sections -fdata-sections -Wall -fstack-usage
#   C++: -fno-exceptions -fno-rtti -fno-use-cxa-atexit
#   ASM: -x assembler-with-cpp
#   link: -static -Wl,--gc-sections -Wl,-Map=<elf>.map, libc libm libstdc++ libsupc++ in a group

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(ARM_SIZE arm-none-eabi-size)

# A bare metal compiler cannot link a test program without a linker script.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_MCU_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb")
set(CMAKE_C_FLAGS_INIT "${ARM_MCU_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${ARM_MCU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${ARM_MCU_FLAGS} -x assembler-with-cpp")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
