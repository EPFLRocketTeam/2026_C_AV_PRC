################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.cpp \
../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.cpp \
../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.cpp 

OBJS += \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.o \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.o \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.o 

CPP_DEPS += \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.d \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.d \
./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/%.o Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/%.su Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/%.cyclo: ../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/%.cpp Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-FC_CAN-2f-2026_C_AV_FC_PRC_INTRANET-2f-src-2f-prc_intranet

clean-Drivers-2f-FC_CAN-2f-2026_C_AV_FC_PRC_INTRANET-2f-src-2f-prc_intranet:
	-$(RM) ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.cyclo ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.d ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.o ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/context.su ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.cyclo ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.d ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.o ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/dispatch.su ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.cyclo ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.d ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.o ./Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/src/prc_intranet/transmit.su

.PHONY: clean-Drivers-2f-FC_CAN-2f-2026_C_AV_FC_PRC_INTRANET-2f-src-2f-prc_intranet

