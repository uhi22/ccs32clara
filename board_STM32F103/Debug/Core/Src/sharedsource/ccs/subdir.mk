################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/canbus.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/connMgr.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/hardwareInterface.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/homeplug.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/ipv6.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/modemFinder.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/myAdc.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/myHelpers.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/myScheduler.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/pevStateMachine.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/pushbutton.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/qca7000.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/tcp.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/temperatures.c \
C:/UwesTechnik/ccs32clara/sharedsource/ccs/udpChecksum.c 

OBJS += \
./Core/Src/sharedsource/ccs/canbus.o \
./Core/Src/sharedsource/ccs/connMgr.o \
./Core/Src/sharedsource/ccs/hardwareInterface.o \
./Core/Src/sharedsource/ccs/homeplug.o \
./Core/Src/sharedsource/ccs/ipv6.o \
./Core/Src/sharedsource/ccs/modemFinder.o \
./Core/Src/sharedsource/ccs/myAdc.o \
./Core/Src/sharedsource/ccs/myHelpers.o \
./Core/Src/sharedsource/ccs/myScheduler.o \
./Core/Src/sharedsource/ccs/pevStateMachine.o \
./Core/Src/sharedsource/ccs/pushbutton.o \
./Core/Src/sharedsource/ccs/qca7000.o \
./Core/Src/sharedsource/ccs/tcp.o \
./Core/Src/sharedsource/ccs/temperatures.o \
./Core/Src/sharedsource/ccs/udpChecksum.o 

C_DEPS += \
./Core/Src/sharedsource/ccs/canbus.d \
./Core/Src/sharedsource/ccs/connMgr.d \
./Core/Src/sharedsource/ccs/hardwareInterface.d \
./Core/Src/sharedsource/ccs/homeplug.d \
./Core/Src/sharedsource/ccs/ipv6.d \
./Core/Src/sharedsource/ccs/modemFinder.d \
./Core/Src/sharedsource/ccs/myAdc.d \
./Core/Src/sharedsource/ccs/myHelpers.d \
./Core/Src/sharedsource/ccs/myScheduler.d \
./Core/Src/sharedsource/ccs/pevStateMachine.d \
./Core/Src/sharedsource/ccs/pushbutton.d \
./Core/Src/sharedsource/ccs/qca7000.d \
./Core/Src/sharedsource/ccs/tcp.d \
./Core/Src/sharedsource/ccs/temperatures.d \
./Core/Src/sharedsource/ccs/udpChecksum.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/sharedsource/ccs/canbus.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/canbus.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/connMgr.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/connMgr.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/hardwareInterface.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/hardwareInterface.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/homeplug.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/homeplug.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/ipv6.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/ipv6.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/modemFinder.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/modemFinder.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/myAdc.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/myAdc.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/myHelpers.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/myHelpers.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/myScheduler.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/myScheduler.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/pevStateMachine.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/pevStateMachine.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/pushbutton.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/pushbutton.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/qca7000.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/qca7000.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/tcp.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/tcp.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/temperatures.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/temperatures.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/ccs/udpChecksum.o: C:/UwesTechnik/ccs32clara/sharedsource/ccs/udpChecksum.c Core/Src/sharedsource/ccs/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I"C:/UwesTechnik/ccs32clara/sharedsource/exi" -I"C:/UwesTechnik/ccs32clara/sharedsource/ccs" -I"C:/UwesTechnik/ccs32clara/board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-sharedsource-2f-ccs

clean-Core-2f-Src-2f-sharedsource-2f-ccs:
	-$(RM) ./Core/Src/sharedsource/ccs/canbus.cyclo ./Core/Src/sharedsource/ccs/canbus.d ./Core/Src/sharedsource/ccs/canbus.o ./Core/Src/sharedsource/ccs/canbus.su ./Core/Src/sharedsource/ccs/connMgr.cyclo ./Core/Src/sharedsource/ccs/connMgr.d ./Core/Src/sharedsource/ccs/connMgr.o ./Core/Src/sharedsource/ccs/connMgr.su ./Core/Src/sharedsource/ccs/hardwareInterface.cyclo ./Core/Src/sharedsource/ccs/hardwareInterface.d ./Core/Src/sharedsource/ccs/hardwareInterface.o ./Core/Src/sharedsource/ccs/hardwareInterface.su ./Core/Src/sharedsource/ccs/homeplug.cyclo ./Core/Src/sharedsource/ccs/homeplug.d ./Core/Src/sharedsource/ccs/homeplug.o ./Core/Src/sharedsource/ccs/homeplug.su ./Core/Src/sharedsource/ccs/ipv6.cyclo ./Core/Src/sharedsource/ccs/ipv6.d ./Core/Src/sharedsource/ccs/ipv6.o ./Core/Src/sharedsource/ccs/ipv6.su ./Core/Src/sharedsource/ccs/modemFinder.cyclo ./Core/Src/sharedsource/ccs/modemFinder.d ./Core/Src/sharedsource/ccs/modemFinder.o ./Core/Src/sharedsource/ccs/modemFinder.su ./Core/Src/sharedsource/ccs/myAdc.cyclo ./Core/Src/sharedsource/ccs/myAdc.d ./Core/Src/sharedsource/ccs/myAdc.o ./Core/Src/sharedsource/ccs/myAdc.su ./Core/Src/sharedsource/ccs/myHelpers.cyclo ./Core/Src/sharedsource/ccs/myHelpers.d ./Core/Src/sharedsource/ccs/myHelpers.o ./Core/Src/sharedsource/ccs/myHelpers.su ./Core/Src/sharedsource/ccs/myScheduler.cyclo ./Core/Src/sharedsource/ccs/myScheduler.d ./Core/Src/sharedsource/ccs/myScheduler.o ./Core/Src/sharedsource/ccs/myScheduler.su ./Core/Src/sharedsource/ccs/pevStateMachine.cyclo ./Core/Src/sharedsource/ccs/pevStateMachine.d ./Core/Src/sharedsource/ccs/pevStateMachine.o ./Core/Src/sharedsource/ccs/pevStateMachine.su ./Core/Src/sharedsource/ccs/pushbutton.cyclo ./Core/Src/sharedsource/ccs/pushbutton.d ./Core/Src/sharedsource/ccs/pushbutton.o ./Core/Src/sharedsource/ccs/pushbutton.su ./Core/Src/sharedsource/ccs/qca7000.cyclo ./Core/Src/sharedsource/ccs/qca7000.d ./Core/Src/sharedsource/ccs/qca7000.o ./Core/Src/sharedsource/ccs/qca7000.su ./Core/Src/sharedsource/ccs/tcp.cyclo ./Core/Src/sharedsource/ccs/tcp.d ./Core/Src/sharedsource/ccs/tcp.o ./Core/Src/sharedsource/ccs/tcp.su ./Core/Src/sharedsource/ccs/temperatures.cyclo ./Core/Src/sharedsource/ccs/temperatures.d ./Core/Src/sharedsource/ccs/temperatures.o ./Core/Src/sharedsource/ccs/temperatures.su ./Core/Src/sharedsource/ccs/udpChecksum.cyclo ./Core/Src/sharedsource/ccs/udpChecksum.d ./Core/Src/sharedsource/ccs/udpChecksum.o ./Core/Src/sharedsource/ccs/udpChecksum.su

.PHONY: clean-Core-2f-Src-2f-sharedsource-2f-ccs

