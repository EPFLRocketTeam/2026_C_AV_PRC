################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.c \
../Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.c 

C_DEPS += \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.d \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.d 

OBJS += \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.o \
./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.o 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/DSP/Source/ControllerFunctions/%.o Drivers/CMSIS/DSP/Source/ControllerFunctions/%.su Drivers/CMSIS/DSP/Source/ControllerFunctions/%.cyclo: ../Drivers/CMSIS/DSP/Source/ControllerFunctions/%.c Drivers/CMSIS/DSP/Source/ControllerFunctions/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H743xx -c -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/DFU/Inc -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I../Drivers/PT1000 -I../Drivers/FDC1004 -I.. -I../Drivers/Valve -I../Drivers/KULITE_CTL190 -I../Drivers/LMT85 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions

clean-Drivers-2f-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions:
	-$(RM) ./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/ControllerFunctions.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_f32.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q15.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_init_q31.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_f32.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q15.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_pid_reset_q31.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_f32.su ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.cyclo ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.d ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.o ./Drivers/CMSIS/DSP/Source/ControllerFunctions/arm_sin_cos_q31.su

.PHONY: clean-Drivers-2f-CMSIS-2f-DSP-2f-Source-2f-ControllerFunctions

