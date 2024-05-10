##
## This file is part of the libopenstm32 project.
##
## Copyright (C) 2009 Uwe Hermann <uwe@hermann-uwe.de>
##
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
##

OUT_DIR      = obj
PREFIX		?= arm-none-eabi
BINARY		= stm32_ccs
SIZE        = $(PREFIX)-size
CC		      = $(PREFIX)-gcc
CPP	      = $(PREFIX)-g++
LD		      = $(PREFIX)-gcc
OBJCOPY		= $(PREFIX)-objcopy
OBJDUMP		= $(PREFIX)-objdump
MKDIR_P     = mkdir -p
CFLAGS		= -Os -Iinclude/ -Ilibopeninv/include -Ilibopencm3/include -Iexi -Iccs \
				 -fno-common -fno-builtin -DSTM32F1 \
				 -mcpu=cortex-m3 -mthumb -std=gnu99 -ffunction-sections -fdata-sections
CPPFLAGS    = -Og -ggdb -Wall -Wextra -Iinclude/ -Ilibopeninv/include -Ilibopencm3/include -Iexi -Iccs \
				-fno-common -std=c++11 -pedantic -DSTM32F1 -DUSART_BAUDRATE=921600 \
				-ffunction-sections -fdata-sections -fno-builtin -fno-rtti -fno-exceptions -fno-unwind-tables -mcpu=cortex-m3 -mthumb
# Check if the variable GITHUB_RUN_NUMBER exists. When running on the github actions running, this
# variable is automatically available.
# Create a compiler define with the content of the variable. Or, if it does not exist, use replacement value 0.
EXTRACOMPILERFLAGS  = $(shell \
	 if [ -z "$$GITHUB_RUN_NUMBER" ]; then echo "-DGITHUB_RUN_NUMBER=0"; else echo "-DGITHUB_RUN_NUMBER=$$GITHUB_RUN_NUMBER"; fi \
	 )

LDSCRIPT	  = linker.ld
LDFLAGS    = -Llibopencm3/lib -T$(LDSCRIPT) -march=armv7 -nostartfiles -Wl,--gc-sections,-Map,linker.map
OBJSL		  = main.o hwinit.o stm32scheduler.o params.o terminal.o terminal_prj.o \
				 my_string.o digio.o my_fp.o printf.o anain.o \
				 param_save.o errormessage.o stm32_can.o canhardware.o canmap.o cansdo.o \
				 picontroller.o terminalcommands.o \
				 ipv6.o tcp.o \
				 connMgr.o modemFinder.o pevStateMachine.o pevStateMachineISO2.o temperatures.o proximitypilot.o acOBC.o \
				 hardwareInterface.o hardwareVariants.o pushbutton.o udpChecksum.o \
				 homeplug.o myHelpers.o qca7000.o \
				 appHandEXIDatatypesDecoder.o ByteStream.o EncoderChannel.o \
				 appHandEXIDatatypesEncoder.o DecoderChannel.o EXIHeaderDecoder.o \
				 appHandEXIDatatypes.o dinEXIDatatypesDecoder.o EXIHeaderEncoder.o \
				 iso2EXIDatatypesDecoder.o iso2EXIDatatypesEncoder.o iso2EXIDatatypes.o \
				 BitInputStream.o dinEXIDatatypesEncoder.o MethodsBag.o \
				 BitOutputStream.o dinEXIDatatypes.o projectExiConnector.o


OBJS     = $(patsubst %.o,obj/%.o, $(OBJSL))
DEPENDS := $(patsubst %.o,obj/%.d, $(OBJSL))
vpath %.c src/ libopeninv/src exi/ ccs/
vpath %.cpp src/ libopeninv/src exi/ ccs/

OPENOCD_BASE	= /usr
OPENOCD		= $(OPENOCD_BASE)/bin/openocd
OPENOCD_SCRIPTS	= $(OPENOCD_BASE)/share/openocd/scripts
OPENOCD_FLASHER	= $(OPENOCD_SCRIPTS)/interface/parport.cfg
OPENOCD_BOARD	= $(OPENOCD_SCRIPTS)/board/olimex_stm32_h103.cfg

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
NULL := 2>/dev/null
endif

# try-run
# Usage: option = $(call try-run, command,option-ok,otherwise)
# Exit code chooses option.
try-run = $(shell set -e;		\
	if ($(1)) >/dev/null 2>&1;	\
	then echo "$(2)";		\
	else echo "$(3)";		\
	fi)

# Test a linker (ld) option and return the gcc link command equivalent
comma := ,
link_command := -Wl$(comma)
ld-option = $(call try-run, $(PREFIX)-ld $(1) -v,$(link_command)$(1))

# Test whether we can suppress a safe warning about rwx segments
# only supported on binutils 2.39 or later
LDFLAGS	+= $(call ld-option,--no-warn-rwx-segments)

all: directories images
Debug:images
Release: images
cleanDebug:clean
images: $(BINARY)
	@printf "  OBJCOPY $(BINARY).bin\n"
	$(Q)$(OBJCOPY) -Obinary $(BINARY) $(BINARY).bin
	@printf "  OBJCOPY $(BINARY).hex\n"
	$(Q)$(OBJCOPY) -Oihex $(BINARY) $(BINARY).hex
	$(Q)$(SIZE) $(BINARY)

directories: ${OUT_DIR}

${OUT_DIR}:
	$(Q)${MKDIR_P} ${OUT_DIR}

$(BINARY): $(OBJS) $(LDSCRIPT)
	@printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(LD) $(LDFLAGS) -o $(BINARY) $(OBJS) -lopencm3_stm32f1 -lm

-include $(DEPENDS)

$(OUT_DIR)/%.o: %.c Makefile
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -MMD -MP -o $@ -c $<

$(OUT_DIR)/%.o: %.cpp Makefile
	@printf "  CPP     $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CPP) $(CPPFLAGS) $(EXTRACOMPILERFLAGS) -MMD -MP -o $@ -c $<

clean:
	@printf "  CLEAN   ${OUT_DIR}\n"
	$(Q)rm -rf ${OUT_DIR}
	@printf "  CLEAN   $(BINARY)\n"
	$(Q)rm -f $(BINARY)
	@printf "  CLEAN   $(BINARY).bin\n"
	$(Q)rm -f $(BINARY).bin
	@printf "  CLEAN   $(BINARY).hex\n"
	$(Q)rm -f $(BINARY).hex
	@printf "  CLEAN   $(BINARY).srec\n"
	$(Q)rm -f $(BINARY).srec
	@printf "  CLEAN   $(BINARY).list\n"
	$(Q)rm -f $(BINARY).list

flash: images
	@printf "  FLASH   $(BINARY).bin\n"
	@# IMPORTANT: Don't use "resume", only "reset" will work correctly!
	$(Q)$(OPENOCD) -s $(OPENOCD_SCRIPTS) \
				 -f $(OPENOCD_FLASHER) \
				 -f $(OPENOCD_BOARD) \
				 -c "init" -c "reset halt" \
				 -c "flash write_image erase $(BINARY).hex" \
				 -c "reset" \
				 -c "shutdown" $(NULL)

.PHONY: directories images clean

get-deps:
	@printf "  GIT SUBMODULE\n"
	$(Q)git submodule update --init
	@printf "  MAKE libopencm3\n"
	$(Q)${MAKE} -C libopencm3 TARGETS=stm32/f1

Test:
	cd test && $(MAKE)
cleanTest:
	cd test && $(MAKE) clean
