################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/Src/exi/BitInputStream.c \
../Core/Src/exi/BitOutputStream.c \
../Core/Src/exi/ByteStream.c \
../Core/Src/exi/DecoderChannel.c \
../Core/Src/exi/EXIHeaderDecoder.c \
../Core/Src/exi/EXIHeaderEncoder.c \
../Core/Src/exi/EncoderChannel.c \
../Core/Src/exi/MethodsBag.c \
../Core/Src/exi/appHandEXIDatatypes.c \
../Core/Src/exi/appHandEXIDatatypesDecoder.c \
../Core/Src/exi/appHandEXIDatatypesEncoder.c \
../Core/Src/exi/dinEXIDatatypes.c \
../Core/Src/exi/dinEXIDatatypesDecoder.c \
../Core/Src/exi/dinEXIDatatypesEncoder.c \
../Core/Src/exi/projectExiConnector.c 

OBJS += \
./Core/Src/exi/BitInputStream.o \
./Core/Src/exi/BitOutputStream.o \
./Core/Src/exi/ByteStream.o \
./Core/Src/exi/DecoderChannel.o \
./Core/Src/exi/EXIHeaderDecoder.o \
./Core/Src/exi/EXIHeaderEncoder.o \
./Core/Src/exi/EncoderChannel.o \
./Core/Src/exi/MethodsBag.o \
./Core/Src/exi/appHandEXIDatatypes.o \
./Core/Src/exi/appHandEXIDatatypesDecoder.o \
./Core/Src/exi/appHandEXIDatatypesEncoder.o \
./Core/Src/exi/dinEXIDatatypes.o \
./Core/Src/exi/dinEXIDatatypesDecoder.o \
./Core/Src/exi/dinEXIDatatypesEncoder.o \
./Core/Src/exi/projectExiConnector.o 

C_DEPS += \
./Core/Src/exi/BitInputStream.d \
./Core/Src/exi/BitOutputStream.d \
./Core/Src/exi/ByteStream.d \
./Core/Src/exi/DecoderChannel.d \
./Core/Src/exi/EXIHeaderDecoder.d \
./Core/Src/exi/EXIHeaderEncoder.d \
./Core/Src/exi/EncoderChannel.d \
./Core/Src/exi/MethodsBag.d \
./Core/Src/exi/appHandEXIDatatypes.d \
./Core/Src/exi/appHandEXIDatatypesDecoder.d \
./Core/Src/exi/appHandEXIDatatypesEncoder.d \
./Core/Src/exi/dinEXIDatatypes.d \
./Core/Src/exi/dinEXIDatatypesDecoder.d \
./Core/Src/exi/dinEXIDatatypesEncoder.d \
./Core/Src/exi/projectExiConnector.d 


# Each subdirectory must supply rules for building sources it contributes
Core/Src/exi/%.o Core/Src/exi/%.su Core/Src/exi/%.cyclo: ../Core/Src/exi/%.c Core/Src/exi/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F303xE -c -I../Core/Inc -I../Drivers/STM32F3xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F3xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32F3xx/Include -I../Drivers/CMSIS/Include -I"C:/UwesTechnik/ccs32clara/Core/Src/exi" -I"C:/UwesTechnik/ccs32clara/Core/Src/ccs" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Core-2f-Src-2f-exi

clean-Core-2f-Src-2f-exi:
	-$(RM) ./Core/Src/exi/BitInputStream.cyclo ./Core/Src/exi/BitInputStream.d ./Core/Src/exi/BitInputStream.o ./Core/Src/exi/BitInputStream.su ./Core/Src/exi/BitOutputStream.cyclo ./Core/Src/exi/BitOutputStream.d ./Core/Src/exi/BitOutputStream.o ./Core/Src/exi/BitOutputStream.su ./Core/Src/exi/ByteStream.cyclo ./Core/Src/exi/ByteStream.d ./Core/Src/exi/ByteStream.o ./Core/Src/exi/ByteStream.su ./Core/Src/exi/DecoderChannel.cyclo ./Core/Src/exi/DecoderChannel.d ./Core/Src/exi/DecoderChannel.o ./Core/Src/exi/DecoderChannel.su ./Core/Src/exi/EXIHeaderDecoder.cyclo ./Core/Src/exi/EXIHeaderDecoder.d ./Core/Src/exi/EXIHeaderDecoder.o ./Core/Src/exi/EXIHeaderDecoder.su ./Core/Src/exi/EXIHeaderEncoder.cyclo ./Core/Src/exi/EXIHeaderEncoder.d ./Core/Src/exi/EXIHeaderEncoder.o ./Core/Src/exi/EXIHeaderEncoder.su ./Core/Src/exi/EncoderChannel.cyclo ./Core/Src/exi/EncoderChannel.d ./Core/Src/exi/EncoderChannel.o ./Core/Src/exi/EncoderChannel.su ./Core/Src/exi/MethodsBag.cyclo ./Core/Src/exi/MethodsBag.d ./Core/Src/exi/MethodsBag.o ./Core/Src/exi/MethodsBag.su ./Core/Src/exi/appHandEXIDatatypes.cyclo ./Core/Src/exi/appHandEXIDatatypes.d ./Core/Src/exi/appHandEXIDatatypes.o ./Core/Src/exi/appHandEXIDatatypes.su ./Core/Src/exi/appHandEXIDatatypesDecoder.cyclo ./Core/Src/exi/appHandEXIDatatypesDecoder.d ./Core/Src/exi/appHandEXIDatatypesDecoder.o ./Core/Src/exi/appHandEXIDatatypesDecoder.su ./Core/Src/exi/appHandEXIDatatypesEncoder.cyclo ./Core/Src/exi/appHandEXIDatatypesEncoder.d ./Core/Src/exi/appHandEXIDatatypesEncoder.o ./Core/Src/exi/appHandEXIDatatypesEncoder.su ./Core/Src/exi/dinEXIDatatypes.cyclo ./Core/Src/exi/dinEXIDatatypes.d ./Core/Src/exi/dinEXIDatatypes.o ./Core/Src/exi/dinEXIDatatypes.su ./Core/Src/exi/dinEXIDatatypesDecoder.cyclo ./Core/Src/exi/dinEXIDatatypesDecoder.d ./Core/Src/exi/dinEXIDatatypesDecoder.o ./Core/Src/exi/dinEXIDatatypesDecoder.su ./Core/Src/exi/dinEXIDatatypesEncoder.cyclo ./Core/Src/exi/dinEXIDatatypesEncoder.d ./Core/Src/exi/dinEXIDatatypesEncoder.o ./Core/Src/exi/dinEXIDatatypesEncoder.su ./Core/Src/exi/projectExiConnector.cyclo ./Core/Src/exi/projectExiConnector.d ./Core/Src/exi/projectExiConnector.o ./Core/Src/exi/projectExiConnector.su

.PHONY: clean-Core-2f-Src-2f-exi

