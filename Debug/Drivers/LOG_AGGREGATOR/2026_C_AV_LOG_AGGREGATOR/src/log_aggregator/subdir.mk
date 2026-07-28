################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.cpp \
../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.cpp 

OBJS += \
./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.o \
./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.o 

CPP_DEPS += \
./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.d \
./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/%.o Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/%.su Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/%.cyclo: ../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/%.cpp Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -I"/home/maxime/Documents/ert/fix/2026_C_AV_PRC/ThirdParty/SignalUtils/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-LOG_AGGREGATOR-2f-2026_C_AV_LOG_AGGREGATOR-2f-src-2f-log_aggregator

clean-Drivers-2f-LOG_AGGREGATOR-2f-2026_C_AV_LOG_AGGREGATOR-2f-src-2f-log_aggregator:
	-$(RM) ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.cyclo ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.d ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.o ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/chunker.su ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.cyclo ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.d ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.o ./Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/src/log_aggregator/reassembler.su

.PHONY: clean-Drivers-2f-LOG_AGGREGATOR-2f-2026_C_AV_LOG_AGGREGATOR-2f-src-2f-log_aggregator

