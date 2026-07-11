################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/SensataPte7300/Impl/SensataPte7300.cpp \
../Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.cpp 

OBJS += \
./Drivers/SensataPte7300/Impl/SensataPte7300.o \
./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.o 

CPP_DEPS += \
./Drivers/SensataPte7300/Impl/SensataPte7300.d \
./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SensataPte7300/Impl/%.o Drivers/SensataPte7300/Impl/%.su Drivers/SensataPte7300/Impl/%.cyclo: ../Drivers/SensataPte7300/Impl/%.cpp Drivers/SensataPte7300/Impl/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-SensataPte7300-2f-Impl

clean-Drivers-2f-SensataPte7300-2f-Impl:
	-$(RM) ./Drivers/SensataPte7300/Impl/SensataPte7300.cyclo ./Drivers/SensataPte7300/Impl/SensataPte7300.d ./Drivers/SensataPte7300/Impl/SensataPte7300.o ./Drivers/SensataPte7300/Impl/SensataPte7300.su ./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.cyclo ./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.d ./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.o ./Drivers/SensataPte7300/Impl/SensataPte7300HardwareTest.su

.PHONY: clean-Drivers-2f-SensataPte7300-2f-Impl

