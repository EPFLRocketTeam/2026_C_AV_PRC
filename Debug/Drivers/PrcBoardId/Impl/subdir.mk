################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Drivers/PrcBoardId/Impl/PrcBoardId.cpp 

OBJS += \
./Drivers/PrcBoardId/Impl/PrcBoardId.o 

CPP_DEPS += \
./Drivers/PrcBoardId/Impl/PrcBoardId.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/PrcBoardId/Impl/%.o Drivers/PrcBoardId/Impl/%.su Drivers/PrcBoardId/Impl/%.cyclo: ../Drivers/PrcBoardId/Impl/%.cpp Drivers/PrcBoardId/Impl/subdir.mk
	arm-none-eabi-g++ "$<" -mcpu=cortex-m7 -std=gnu++14 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -I../Drivers/FC_CAN/2026_C_AV_FC_PRC_INTRANET/include -I../Drivers/LOG_AGGREGATOR/2026_C_AV_LOG_AGGREGATOR/include -O0 -ffunction-sections -fdata-sections -fno-exceptions -fno-rtti -fno-use-cxa-atexit -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-PrcBoardId-2f-Impl

clean-Drivers-2f-PrcBoardId-2f-Impl:
	-$(RM) ./Drivers/PrcBoardId/Impl/PrcBoardId.cyclo ./Drivers/PrcBoardId/Impl/PrcBoardId.d ./Drivers/PrcBoardId/Impl/PrcBoardId.o ./Drivers/PrcBoardId/Impl/PrcBoardId.su

.PHONY: clean-Drivers-2f-PrcBoardId-2f-Impl

