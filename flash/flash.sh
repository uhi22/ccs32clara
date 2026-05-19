#!/bin/bash

openocd -f interface/stlink-v2.cfg -f board/olimex_stm32_h103.cfg -c "program stm32_canloader.hex" -c "program ../stm32_ccs.hex" -c "flash write_image erase unlock config-foccci.bin 0x0807F800" -c reset -c shutdown
