# PRC firmware ELF, mirroring the Debug configuration of .cproject.
# Included from the root CMakeLists.txt only when cross compiling with
# cmake/arm-none-eabi.cmake; the header of that file lists what was extracted.

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(FATAL_ERROR "Only the .cproject Debug configuration is modelled; pass -DCMAKE_BUILD_TYPE=Debug")
endif()

enable_language(ASM)

# The .cproject sets optimisation and debug level itself; drop CMake's defaults.
set(CMAKE_C_FLAGS_DEBUG "")
set(CMAKE_CXX_FLAGS_DEBUG "")
set(CMAKE_ASM_FLAGS_DEBUG "")

set(FW_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")

# Glob a .cproject source folder and drop its Eclipse exclusion patterns
# (each pattern names a path relative to the folder, and everything under it).
function(fw_sources out folder)
    file(GLOB_RECURSE found RELATIVE "${FW_ROOT}" CONFIGURE_DEPENDS
        "${folder}/*.c" "${folder}/*.cpp" "${folder}/*.cc" "${folder}/*.cxx"
        "${folder}/*.s" "${folder}/*.S")
    foreach(pattern IN LISTS ARGN)
        string(REPLACE "." "\\." pattern "${pattern}")
        list(FILTER found EXCLUDE REGEX "^${folder}/${pattern}(/|$)")
    endforeach()
    list(TRANSFORM found PREPEND "${FW_ROOT}/")
    set(${out} ${${out}} ${found} PARENT_SCOPE)
endfunction()

set(FW_SOURCES "")
fw_sources(FW_SOURCES Application)
fw_sources(FW_SOURCES Core)
fw_sources(FW_SOURCES Drivers
    Plume/2026_C_AV_PLUME/src/plume/cli
    Plume/2026_C_AV_PLUME/src/plume/drivers
    Plume/2026_C_AV_PLUME/tests
    STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_msp_template.c
    STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_wakeup_template.c
    STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_timebase_rtc_alarm_template.c
    CMSIS/DSP
    CMSIS/Device/ST/STM32H7xx/Source
    CMSIS/Core/Template
    CMSIS/Core_A/Source
    CMSIS/NN
    CMSIS/RTOS2)
fw_sources(FW_SOURCES Middlewares
    ST/STM32_USB_Device_Library/Core/Src/usbd_desc_template.c
    ST/STM32_USB_Device_Library/Core/Src/usbd_conf_template.c)
fw_sources(FW_SOURCES Modules Sensors/tests)
fw_sources(FW_SOURCES USB_DEVICE)

set(FW_C_INCLUDES
    USB_DEVICE/App
    Application/Config/2026_C_AV_FLIGHT_PARAMS/2026_C_AV_CONFMAN
    Application/Config/2026_C_AV_FLIGHT_PARAMS
    USB_DEVICE/Target
    Core/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc
    Drivers/CMSIS/Device/ST/STM32H7xx/Include
    Drivers/CMSIS/Include
    Drivers/PT1000
    Drivers/FDC1004
    .
    Drivers/Valve
    Drivers/KULITE_CTL190
    Drivers/LMT85
    ThirdParty/SignalUtils/include
    Modules
    Drivers/Plume/2026_C_AV_PLUME/include)

set(FW_CXX_INCLUDES
    USB_DEVICE/App
    Application/Config/2026_C_AV_FLIGHT_PARAMS/2026_C_AV_CONFMAN
    Application/Config/2026_C_AV_FLIGHT_PARAMS
    USB_DEVICE/Target
    Core/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc
    Drivers/STM32H7xx_HAL_Driver/Inc/Legacy
    Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc
    Drivers/CMSIS/Device/ST/STM32H7xx/Include
    Drivers/CMSIS/Include
    Drivers/PT1000
    Drivers/FDC1004
    .
    Drivers/Valve
    Drivers/KULITE_CTL190
    Drivers/LMT85
    Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include
    Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include
    ThirdParty/SignalUtils/include
    Modules
    Drivers/Plume/2026_C_AV_PLUME/include)

set(FW_ASM_INCLUDES
    Drivers/PT1000
    Drivers/FDC1004
    ThirdParty/SignalUtils/include
    Drivers/Plume/2026_C_AV_PLUME/include)

set(FW_DEFINES
    DEBUG
    USE_PWR_LDO_SUPPLY
    USE_HAL_DRIVER
    STM32H743xx)

list(TRANSFORM FW_C_INCLUDES PREPEND "${FW_ROOT}/")
list(TRANSFORM FW_CXX_INCLUDES PREPEND "${FW_ROOT}/")
list(TRANSFORM FW_ASM_INCLUDES PREPEND "${FW_ROOT}/")

set(FW_NAME 2026_C_AV_PRC_IOC)
add_executable(${FW_NAME} ${FW_SOURCES})
set_target_properties(${FW_NAME} PROPERTIES
    SUFFIX ".elf"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON)

target_include_directories(${FW_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:${FW_C_INCLUDES}>"
    "$<$<COMPILE_LANGUAGE:CXX>:${FW_CXX_INCLUDES}>"
    "$<$<COMPILE_LANGUAGE:ASM>:${FW_ASM_INCLUDES}>")

target_compile_definitions(${FW_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:C,CXX>:${FW_DEFINES}>"
    "$<$<COMPILE_LANGUAGE:ASM>:DEBUG>")

target_compile_options(${FW_NAME} PRIVATE
    -g3
    "$<$<COMPILE_LANGUAGE:C,CXX>:-O0;-ffunction-sections;-fdata-sections;-Wall;-fstack-usage>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions;-fno-rtti;-fno-use-cxa-atexit>"
    "$<$<COMPILE_LANGUAGE:C,CXX>:--specs=nano.specs>")

target_link_options(${FW_NAME} PRIVATE
    -T${FW_ROOT}/STM32H743VITX_FLASH.ld
    --specs=nano.specs
    --specs=nosys.specs
    -static
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${FW_NAME}.map
    -Wl,--gc-sections
    "SHELL:-u _printf_float")
target_link_libraries(${FW_NAME} PRIVATE
    -Wl,--start-group c m stdc++ supc++ -Wl,--end-group)

add_custom_command(TARGET ${FW_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${FW_NAME}> ${FW_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${FW_NAME}> ${FW_NAME}.bin
    COMMAND ${ARM_SIZE} $<TARGET_FILE:${FW_NAME}>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM)
