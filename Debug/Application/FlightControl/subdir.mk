################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Application/FlightControl/prc_can.cpp \
../Application/FlightControl/prc_state.cpp 

OBJS += \
./Application/FlightControl/prc_can.o \
./Application/FlightControl/prc_state.o 

CPP_DEPS += \
./Application/FlightControl/prc_can.d \
./Application/FlightControl/prc_state.d 


# Each subdirectory must supply rules for building sources it contributes
Application/FlightControl/%.o Application/FlightControl/%.su Application/FlightControl/%.cyclo: ../Application/FlightControl/%.cpp Application/FlightControl/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-FlightControl

clean-Application-2f-FlightControl:
	-$(RM) ./Application/FlightControl/prc_can.cyclo ./Application/FlightControl/prc_can.d ./Application/FlightControl/prc_can.o ./Application/FlightControl/prc_can.su ./Application/FlightControl/prc_state.cyclo ./Application/FlightControl/prc_state.d ./Application/FlightControl/prc_state.o ./Application/FlightControl/prc_state.su

.PHONY: clean-Application-2f-FlightControl

