################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Application/Data/Stores/BoardIdentityStore.cpp \
../Application/Data/Stores/EventStore.cpp \
../Application/Data/Stores/UplinkCmdStore.cpp \
../Application/Data/Stores/ValvesStore.cpp 

OBJS += \
./Application/Data/Stores/BoardIdentityStore.o \
./Application/Data/Stores/EventStore.o \
./Application/Data/Stores/UplinkCmdStore.o \
./Application/Data/Stores/ValvesStore.o 

CPP_DEPS += \
./Application/Data/Stores/BoardIdentityStore.d \
./Application/Data/Stores/EventStore.d \
./Application/Data/Stores/UplinkCmdStore.d \
./Application/Data/Stores/ValvesStore.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Data/Stores/%.o Application/Data/Stores/%.su Application/Data/Stores/%.cyclo: ../Application/Data/Stores/%.cpp Application/Data/Stores/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -I"/home/maxime/Documents/ert/fix/2026_C_AV_PRC/ThirdParty/SignalUtils/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Data-2f-Stores

clean-Application-2f-Data-2f-Stores:
	-$(RM) ./Application/Data/Stores/BoardIdentityStore.cyclo ./Application/Data/Stores/BoardIdentityStore.d ./Application/Data/Stores/BoardIdentityStore.o ./Application/Data/Stores/BoardIdentityStore.su ./Application/Data/Stores/EventStore.cyclo ./Application/Data/Stores/EventStore.d ./Application/Data/Stores/EventStore.o ./Application/Data/Stores/EventStore.su ./Application/Data/Stores/UplinkCmdStore.cyclo ./Application/Data/Stores/UplinkCmdStore.d ./Application/Data/Stores/UplinkCmdStore.o ./Application/Data/Stores/UplinkCmdStore.su ./Application/Data/Stores/ValvesStore.cyclo ./Application/Data/Stores/ValvesStore.d ./Application/Data/Stores/ValvesStore.o ./Application/Data/Stores/ValvesStore.su

.PHONY: clean-Application-2f-Data-2f-Stores

