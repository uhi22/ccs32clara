/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <libopencm3/cm3/common.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/adc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/crc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/desig.h>
#include "hwdefs.h"
#include "hwinit.h"
#include "stm32_loader.h"
#include "my_string.h"

/**
* Start clocks of all needed peripherals
*/
void clock_setup(void)
{
   RCC_CLOCK_SETUP();

   //The reset value for PRIGROUP (=0) is not actually a defined
   //value. Explicitly set 16 preemtion priorities
   SCB_AIRCR = SCB_AIRCR_VECTKEY | SCB_AIRCR_PRIGROUP_GROUP16_NOSUB;

   rcc_periph_clock_enable(RCC_GPIOA);
   rcc_periph_clock_enable(RCC_GPIOB);
   rcc_periph_clock_enable(RCC_GPIOC);
   rcc_periph_clock_enable(RCC_GPIOD);
   rcc_periph_clock_enable(RCC_UART4);
   rcc_periph_clock_enable(RCC_TIM1); //Don't know
   rcc_periph_clock_enable(RCC_TIM2); //CP measurement
   rcc_periph_clock_enable(RCC_TIM3); //Contactor and lock driver
   rcc_periph_clock_enable(RCC_TIM4); //Scheduler
   rcc_periph_clock_enable(RCC_DMA1); //for SPI
   rcc_periph_clock_enable(RCC_DMA2); //For UART 4
   rcc_periph_clock_enable(RCC_ADC1);
   rcc_periph_clock_enable(RCC_CRC);
   rcc_periph_clock_enable(RCC_AFIO); //CAN
   rcc_periph_clock_enable(RCC_CAN1); //CAN
   rcc_periph_clock_enable(RCC_SPI1); //QCA comms
}

/* Some pins should never be left floating at any time
 * Since the bootloader delays firmware startup by a few 100ms
 * We need to tell it which pins we want to initialize right
 * after startup
 */
void write_bootloader_pininit()
{
   uint32_t flashSize = desig_get_flash_size();
   uint32_t pindefAddr = FLASH_BASE + flashSize * 1024 - PINDEF_BLKNUM * PINDEF_BLKSIZE;
   const struct pincommands* flashCommands = (struct pincommands*)pindefAddr;

   struct pincommands commands;

   memset32((int*)&commands, 0, PINDEF_NUMWORDS);

   //!!! Customize this to match your project !!!
   //Make sure stateC, contactor and lock doesn't float
   commands.pindef[1].port = GPIOB;
   commands.pindef[1].pin = GPIO4;
   commands.pindef[1].inout = PIN_OUT;
   commands.pindef[1].level = 0;
   commands.pindef[1].port = GPIOC;
   commands.pindef[1].pin = GPIO6 | GPIO7 | GPIO8 | GPIO9;
   commands.pindef[1].inout = PIN_OUT;
   commands.pindef[1].level = 0;

   crc_reset();
   uint32_t crc = crc_calculate_block(((uint32_t*)&commands), PINDEF_NUMWORDS);
   commands.crc = crc;

   if (commands.crc != flashCommands->crc)
   {
      flash_unlock();
      flash_erase_page(pindefAddr);

      //Write flash including crc, therefor <=
      for (uint32_t idx = 0; idx <= PINDEF_NUMWORDS; idx++)
      {
         uint32_t* pData = ((uint32_t*)&commands) + idx;
         flash_program_word(pindefAddr + idx * sizeof(uint32_t), *pData);
      }
      flash_lock();
   }
}

/**
* Enable Timer refresh and break interrupts
*/
void nvic_setup(void)
{
   nvic_enable_irq(NVIC_TIM4_IRQ); //Scheduler
   nvic_set_priority(NVIC_TIM4_IRQ, 0xe << 4); //second lowest priority
   nvic_enable_irq(NVIC_DMA1_CHANNEL2_IRQ); //SPI RX complete
   nvic_set_priority(NVIC_DMA1_CHANNEL2_IRQ, 0xd << 4); //third lowest priority
}

void rtc_setup()
{
   //Base clock is HSE/128 = 8MHz/128 = 62.5kHz
   //62.5kHz / (624 + 1) = 100Hz
   rtc_auto_awake(RCC_HSE, 624); //10ms tick
   rtc_set_counter_val(0);
}

/**
* returns the number of milliseconds since startup
*/
uint32_t rtc_get_ms(void) {
    return rtc_get_counter_val()*10; /* The RTC has a 10ms tick (see rtc_setup()). So we multiply by 10 to get the milliseconds. */
}

/**
* Setup CONTACT_LOCK_TIMER to switch the port relays (with PWM economizer)
* and the charge port lock motor
* Setup CP_TIMER to measure the pulse width of the CP signal
*/
void tim_setup(int variant)
{
   //The slow switching NCV8402 forces us to switch at 500 Hz
   //Later revisions can be switched at 17.6 kHz
   int prescaler = variant <= 4003 ? 34 : 0;
   /*** Setup over/undercurrent and PWM output timer */
   timer_disable_counter(CONTACT_LOCK_TIMER);
   //edge aligned PWM
   timer_set_alignment(CONTACT_LOCK_TIMER, TIM_CR1_CMS_EDGE);
   timer_enable_preload(CONTACT_LOCK_TIMER);
   /* PWM mode 1 and preload enable */
   timer_set_oc_mode(CONTACT_LOCK_TIMER, TIM_OC1, TIM_OCM_PWM1);
   timer_set_oc_mode(CONTACT_LOCK_TIMER, TIM_OC2, TIM_OCM_PWM1);
   timer_set_oc_mode(CONTACT_LOCK_TIMER, TIM_OC3, TIM_OCM_PWM1);
   timer_set_oc_mode(CONTACT_LOCK_TIMER, TIM_OC4, TIM_OCM_PWM1);
   timer_enable_oc_preload(CONTACT_LOCK_TIMER, TIM_OC1);
   timer_enable_oc_preload(CONTACT_LOCK_TIMER, TIM_OC2);
   timer_enable_oc_preload(CONTACT_LOCK_TIMER, TIM_OC3);
   timer_enable_oc_preload(CONTACT_LOCK_TIMER, TIM_OC4);

   timer_set_oc_polarity_high(CONTACT_LOCK_TIMER, TIM_OC1);
   timer_set_oc_polarity_high(CONTACT_LOCK_TIMER, TIM_OC2);
   timer_set_oc_polarity_high(CONTACT_LOCK_TIMER, TIM_OC3);
   timer_set_oc_polarity_high(CONTACT_LOCK_TIMER, TIM_OC4);
   timer_enable_oc_output(CONTACT_LOCK_TIMER, TIM_OC1);
   timer_enable_oc_output(CONTACT_LOCK_TIMER, TIM_OC2);
   timer_enable_oc_output(CONTACT_LOCK_TIMER, TIM_OC3);
   timer_enable_oc_output(CONTACT_LOCK_TIMER, TIM_OC4);
   timer_generate_event(CONTACT_LOCK_TIMER, TIM_EGR_UG);
   timer_set_prescaler(CONTACT_LOCK_TIMER, prescaler);
   /* PWM frequency */
   timer_set_period(CONTACT_LOCK_TIMER, CONTACT_LOCK_PERIOD);
   timer_enable_counter(CONTACT_LOCK_TIMER);

   timer_set_prescaler(CP_TIMER, 71); //run at 1 MHz
   timer_set_period(CP_TIMER, 65535);
   timer_direction_up(CP_TIMER);
   timer_slave_set_mode(CP_TIMER, TIM_SMCR_SMS_RM);
   timer_slave_set_polarity(CP_TIMER, TIM_ET_FALLING);
   timer_slave_set_trigger(CP_TIMER, TIM_SMCR_TS_TI1FP1);
   timer_ic_set_filter(CP_TIMER, TIM_IC1, TIM_IC_DTF_DIV_32_N_8);
   timer_ic_set_filter(CP_TIMER, TIM_IC2, TIM_IC_DTF_DIV_32_N_8);
   timer_ic_set_input(CP_TIMER, TIM_IC1, TIM_IC_IN_TI1);//measures frequency
   timer_ic_set_input(CP_TIMER, TIM_IC2, TIM_IC_IN_TI1);//measure duty cycle
   timer_set_oc_polarity_high(CP_TIMER, TIM_OC1);
   timer_set_oc_polarity_low(CP_TIMER, TIM_OC2);
   timer_ic_enable(CP_TIMER, TIM_IC1);
   timer_ic_enable(CP_TIMER, TIM_IC2);
   timer_generate_event(CP_TIMER, TIM_EGR_UG);
   timer_enable_counter(CP_TIMER);

   /** setup gpio */
   gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6 | GPIO7 | GPIO8 | GPIO9);
   gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO15);
}

