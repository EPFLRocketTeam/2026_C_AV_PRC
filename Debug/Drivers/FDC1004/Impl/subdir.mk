################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/FDC1004/Impl/FDC1004.cpp \
../Drivers/FDC1004/Impl/FDC1004_manual_test.cpp 

OBJS += \
./Drivers/FDC1004/Impl/FDC1004.o \
./Drivers/FDC1004/Impl/FDC1004_manual_test.o 

CPP_DEPS += \
./Drivers/FDC1004/Impl/FDC1004.d \
./Drivers/FDC1004/Impl/FDC1004_manual_test.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/FDC1004/Impl/%.o Drivers/FDC1004/Impl/%.su Drivers/FDC1004/Impl/%.cyclo: ../Drivers/FDC1004/Impl/%.cpp Drivers/FDC1004/Impl/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++17 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -I"/home/maxime/Documents/ert/fix/2026_C_AV_PRC/ThirdParty/SignalUtils/include" -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-FDC1004-2f-Impl

clean-Drivers-2f-FDC1004-2f-Impl:
	-$(RM) ./Drivers/FDC1004/Impl/FDC1004.cyclo ./Drivers/FDC1004/Impl/FDC1004.d ./Drivers/FDC1004/Impl/FDC1004.o ./Drivers/FDC1004/Impl/FDC1004.su ./Drivers/FDC1004/Impl/FDC1004_manual_test.cyclo ./Drivers/FDC1004/Impl/FDC1004_manual_test.d ./Drivers/FDC1004/Impl/FDC1004_manual_test.o ./Drivers/FDC1004/Impl/FDC1004_manual_test.su

.PHONY: clean-Drivers-2f-FDC1004-2f-Impl

