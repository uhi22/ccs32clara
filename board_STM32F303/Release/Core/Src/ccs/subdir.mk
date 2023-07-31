################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/ccs/connMgr.c \
../Core/Src/ccs/hardwareInterface.c \
../Core/Src/ccs/homeplug.c \
../Core/Src/ccs/ipv6.c \
../Core/Src/ccs/modemFinder.c \
../Core/Src/ccs/myHelpers.c \
../Core/Src/ccs/myScheduler.c \
../Core/Src/ccs/pevStateMachine.c \
../Core/Src/ccs/qca7000.c \
../Core/Src/ccs/tcp.c \
../Core/Src/ccs/udpChecksum.c 

OBJS += \
./Core/Src/ccs/connMgr.o \
./Core/Src/ccs/hardwareInterface.o \
./Core/Src/ccs/homeplug.o \
./Core/Src/ccs/ipv6.o \
./Core/Src/ccs/modemFinder.o \
./Core/Src/ccs/myHelpers.o \
./Core/Src/ccs/myScheduler.o \
./Core/Src/ccs/pevStateMachine.o \
./Core/Src/ccs/qca7000.o \
./Core/Src/ccs/tcp.o \
./Core/Src/ccs/udpChecksum.o 

C_DEPS += \
./Core/Src/ccs/connMgr.d \
./Core/Src/ccs/hardwareInterface.d \
./Core/Src/ccs/homeplug.d \
./Core/Src/ccs/ipv6.d \
./Core/Src/ccs/modemFinder.d \
./Core/Src/ccs/myHelpers.d \
./Core/Src/ccs/myScheduler.d \
./Core/Src/ccs/pevStateMachine.d \
./Core/Src/ccs/qca7000.d \
./Core/Src/ccs/tcp.d \
./Core/Src/ccs/udpChecksum.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/ccs/%.o Core/Src/ccs/%.su Core/Src/ccs/%.cyclo: ../Core/Src/ccs/%.c Core/Src/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F303xE -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -I"C:/UwesTechnik/ccs32clara/Core/Src/exi" -Os -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-ccs

clean-Core-2f-Src-2f-ccs:
	-$(RM) ./Core/Src/ccs/connMgr.cyclo ./Core/Src/ccs/connMgr.d ./Core/Src/ccs/connMgr.o ./Core/Src/ccs/connMgr.su ./Core/Src/ccs/hardwareInterface.cyclo ./Core/Src/ccs/hardwareInterface.d ./Core/Src/ccs/hardwareInterface.o ./Core/Src/ccs/hardwareInterface.su ./Core/Src/ccs/homeplug.cyclo ./Core/Src/ccs/homeplug.d ./Core/Src/ccs/homeplug.o ./Core/Src/ccs/homeplug.su ./Core/Src/ccs/ipv6.cyclo ./Core/Src/ccs/ipv6.d ./Core/Src/ccs/ipv6.o ./Core/Src/ccs/ipv6.su ./Core/Src/ccs/modemFinder.cyclo ./Core/Src/ccs/modemFinder.d ./Core/Src/ccs/modemFinder.o ./Core/Src/ccs/modemFinder.su ./Core/Src/ccs/myHelpers.cyclo ./Core/Src/ccs/myHelpers.d ./Core/Src/ccs/myHelpers.o ./Core/Src/ccs/myHelpers.su ./Core/Src/ccs/myScheduler.cyclo ./Core/Src/ccs/myScheduler.d ./Core/Src/ccs/myScheduler.o ./Core/Src/ccs/myScheduler.su ./Core/Src/ccs/pevStateMachine.cyclo ./Core/Src/ccs/pevStateMachine.d ./Core/Src/ccs/pevStateMachine.o ./Core/Src/ccs/pevStateMachine.su ./Core/Src/ccs/qca7000.cyclo ./Core/Src/ccs/qca7000.d ./Core/Src/ccs/qca7000.o ./Core/Src/ccs/qca7000.su ./Core/Src/ccs/tcp.cyclo ./Core/Src/ccs/tcp.d ./Core/Src/ccs/tcp.o ./Core/Src/ccs/tcp.su ./Core/Src/ccs/udpChecksum.cyclo ./Core/Src/ccs/udpChecksum.d ./Core/Src/ccs/udpChecksum.o ./Core/Src/ccs/udpChecksum.su

.PHONY: clean-Core-2f-Src-2f-ccs

