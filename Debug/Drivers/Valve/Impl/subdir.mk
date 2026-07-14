################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/Valve/Impl/Valve.cpp \
../Drivers/Valve/Impl/ValveList.cpp \
../Drivers/Valve/Impl/valve_manual_test.cpp 

OBJS += \
./Drivers/Valve/Impl/Valve.o \
./Drivers/Valve/Impl/ValveList.o \
./Drivers/Valve/Impl/valve_manual_test.o 

CPP_DEPS += \
./Drivers/Valve/Impl/Valve.d \
./Drivers/Valve/Impl/ValveList.d \
./Drivers/Valve/Impl/valve_manual_test.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Valve/Impl/%.o Drivers/Valve/Impl/%.su Drivers/Valve/Impl/%.cyclo: ../Drivers/Valve/Impl/%.cpp Drivers/Valve/Impl/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Valve-2f-Impl

clean-Drivers-2f-Valve-2f-Impl:
	-$(RM) ./Drivers/Valve/Impl/Valve.cyclo ./Drivers/Valve/Impl/Valve.d ./Drivers/Valve/Impl/Valve.o ./Drivers/Valve/Impl/Valve.su ./Drivers/Valve/Impl/ValveList.cyclo ./Drivers/Valve/Impl/ValveList.d ./Drivers/Valve/Impl/ValveList.o ./Drivers/Valve/Impl/ValveList.su ./Drivers/Valve/Impl/valve_manual_test.cyclo ./Drivers/Valve/Impl/valve_manual_test.d ./Drivers/Valve/Impl/valve_manual_test.o ./Drivers/Valve/Impl/valve_manual_test.su

.PHONY: clean-Drivers-2f-Valve-2f-Impl

