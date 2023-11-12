################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../../sharedsource/exi/BitInputStream.c \
../../sharedsource/exi/BitOutputStream.c \
../../sharedsource/exi/ByteStream.c \
../../sharedsource/exi/DecoderChannel.c \
../../sharedsource/exi/EXIHeaderDecoder.c \
../../sharedsource/exi/EXIHeaderEncoder.c \
../../sharedsource/exi/EncoderChannel.c \
../../sharedsource/exi/MethodsBag.c \
../../sharedsource/exi/appHandEXIDatatypes.c \
../../sharedsource/exi/appHandEXIDatatypesDecoder.c \
../../sharedsource/exi/appHandEXIDatatypesEncoder.c \
../../sharedsource/exi/dinEXIDatatypes.c \
../../sharedsource/exi/dinEXIDatatypesDecoder.c \
../../sharedsource/exi/dinEXIDatatypesEncoder.c \
../../sharedsource/exi/projectExiConnector.c 

OBJS += \
./Core/Src/sharedsource/exi/BitInputStream.o \
./Core/Src/sharedsource/exi/BitOutputStream.o \
./Core/Src/sharedsource/exi/ByteStream.o \
./Core/Src/sharedsource/exi/DecoderChannel.o \
./Core/Src/sharedsource/exi/EXIHeaderDecoder.o \
./Core/Src/sharedsource/exi/EXIHeaderEncoder.o \
./Core/Src/sharedsource/exi/EncoderChannel.o \
./Core/Src/sharedsource/exi/MethodsBag.o \
./Core/Src/sharedsource/exi/appHandEXIDatatypes.o \
./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.o \
./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.o \
./Core/Src/sharedsource/exi/dinEXIDatatypes.o \
./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.o \
./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.o \
./Core/Src/sharedsource/exi/projectExiConnector.o 

C_DEPS += \
./Core/Src/sharedsource/exi/BitInputStream.d \
./Core/Src/sharedsource/exi/BitOutputStream.d \
./Core/Src/sharedsource/exi/ByteStream.d \
./Core/Src/sharedsource/exi/DecoderChannel.d \
./Core/Src/sharedsource/exi/EXIHeaderDecoder.d \
./Core/Src/sharedsource/exi/EXIHeaderEncoder.d \
./Core/Src/sharedsource/exi/EncoderChannel.d \
./Core/Src/sharedsource/exi/MethodsBag.d \
./Core/Src/sharedsource/exi/appHandEXIDatatypes.d \
./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.d \
./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.d \
./Core/Src/sharedsource/exi/dinEXIDatatypes.d \
./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.d \
./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.d \
./Core/Src/sharedsource/exi/projectExiConnector.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/sharedsource/exi/BitInputStream.o: ../../sharedsource/exi/BitInputStream.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/BitOutputStream.o: ../../sharedsource/exi/BitOutputStream.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/ByteStream.o: ../../sharedsource/exi/ByteStream.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/DecoderChannel.o: ../../sharedsource/exi/DecoderChannel.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/EXIHeaderDecoder.o: ../../sharedsource/exi/EXIHeaderDecoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/EXIHeaderEncoder.o: ../../sharedsource/exi/EXIHeaderEncoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/EncoderChannel.o: ../../sharedsource/exi/EncoderChannel.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/MethodsBag.o: ../../sharedsource/exi/MethodsBag.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/appHandEXIDatatypes.o: ../../sharedsource/exi/appHandEXIDatatypes.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.o: ../../sharedsource/exi/appHandEXIDatatypesDecoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.o: ../../sharedsource/exi/appHandEXIDatatypesEncoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/dinEXIDatatypes.o: ../../sharedsource/exi/dinEXIDatatypes.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.o: ../../sharedsource/exi/dinEXIDatatypesDecoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.o: ../../sharedsource/exi/dinEXIDatatypesEncoder.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/Src/sharedsource/exi/projectExiConnector.o: ../../sharedsource/exi/projectExiConnector.c Core/Src/sharedsource/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xE -c -I"../../board_STM32F103/Core/Src" -I"../../sharedsource/exi" -I"../../sharedsource/ccs" -I"../../board_STM32F103/Core/Src" -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage  -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-sharedsource-2f-exi

clean-Core-2f-Src-2f-sharedsource-2f-exi:
	-$(RM) ./Core/Src/sharedsource/exi/BitInputStream.cyclo ./Core/Src/sharedsource/exi/BitInputStream.d ./Core/Src/sharedsource/exi/BitInputStream.o ./Core/Src/sharedsource/exi/BitInputStream.su ./Core/Src/sharedsource/exi/BitOutputStream.cyclo ./Core/Src/sharedsource/exi/BitOutputStream.d ./Core/Src/sharedsource/exi/BitOutputStream.o ./Core/Src/sharedsource/exi/BitOutputStream.su ./Core/Src/sharedsource/exi/ByteStream.cyclo ./Core/Src/sharedsource/exi/ByteStream.d ./Core/Src/sharedsource/exi/ByteStream.o ./Core/Src/sharedsource/exi/ByteStream.su ./Core/Src/sharedsource/exi/DecoderChannel.cyclo ./Core/Src/sharedsource/exi/DecoderChannel.d ./Core/Src/sharedsource/exi/DecoderChannel.o ./Core/Src/sharedsource/exi/DecoderChannel.su ./Core/Src/sharedsource/exi/EXIHeaderDecoder.cyclo ./Core/Src/sharedsource/exi/EXIHeaderDecoder.d ./Core/Src/sharedsource/exi/EXIHeaderDecoder.o ./Core/Src/sharedsource/exi/EXIHeaderDecoder.su ./Core/Src/sharedsource/exi/EXIHeaderEncoder.cyclo ./Core/Src/sharedsource/exi/EXIHeaderEncoder.d ./Core/Src/sharedsource/exi/EXIHeaderEncoder.o ./Core/Src/sharedsource/exi/EXIHeaderEncoder.su ./Core/Src/sharedsource/exi/EncoderChannel.cyclo ./Core/Src/sharedsource/exi/EncoderChannel.d ./Core/Src/sharedsource/exi/EncoderChannel.o ./Core/Src/sharedsource/exi/EncoderChannel.su ./Core/Src/sharedsource/exi/MethodsBag.cyclo ./Core/Src/sharedsource/exi/MethodsBag.d ./Core/Src/sharedsource/exi/MethodsBag.o ./Core/Src/sharedsource/exi/MethodsBag.su ./Core/Src/sharedsource/exi/appHandEXIDatatypes.cyclo ./Core/Src/sharedsource/exi/appHandEXIDatatypes.d ./Core/Src/sharedsource/exi/appHandEXIDatatypes.o ./Core/Src/sharedsource/exi/appHandEXIDatatypes.su ./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.cyclo ./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.d ./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.o ./Core/Src/sharedsource/exi/appHandEXIDatatypesDecoder.su ./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.cyclo ./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.d ./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.o ./Core/Src/sharedsource/exi/appHandEXIDatatypesEncoder.su ./Core/Src/sharedsource/exi/dinEXIDatatypes.cyclo ./Core/Src/sharedsource/exi/dinEXIDatatypes.d ./Core/Src/sharedsource/exi/dinEXIDatatypes.o ./Core/Src/sharedsource/exi/dinEXIDatatypes.su ./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.cyclo ./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.d ./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.o ./Core/Src/sharedsource/exi/dinEXIDatatypesDecoder.su ./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.cyclo ./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.d ./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.o ./Core/Src/sharedsource/exi/dinEXIDatatypesEncoder.su ./Core/Src/sharedsource/exi/projectExiConnector.cyclo ./Core/Src/sharedsource/exi/projectExiConnector.d ./Core/Src/sharedsource/exi/projectExiConnector.o ./Core/Src/sharedsource/exi/projectExiConnector.su

.PHONY: clean-Core-2f-Src-2f-sharedsource-2f-exi

