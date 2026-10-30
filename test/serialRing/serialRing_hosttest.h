/* Host test (test/serialRing): what ccs/serialRing.cpp uses from the hardware and the clocks, as a simulation that
   ring_test.cpp drives. Built with -DSERIALRING_HOST_TEST; nothing here is in the firmware. */

#ifndef SERIALRING_HOSTTEST_H
#define SERIALRING_HOSTTEST_H

#include <stdint.h>

uint32_t rtc_get_ms(void);

/* DMA2 channel 5 (UART4 TX) */
#define DMA2 2u
#define DMA_CHANNEL5 5u
#define DMA_TCIF (1u << 1)
void dma_disable_channel(uint32_t dma, uint8_t channel);
void dma_enable_channel(uint32_t dma, uint8_t channel);
void dma_set_memory_address(uint32_t dma, uint8_t channel, uint32_t address);
void dma_set_number_of_data(uint32_t dma, uint8_t channel, uint16_t number);
void dma_clear_interrupt_flags(uint32_t dma, uint8_t channel, uint32_t interrupts);
void dma_enable_transfer_complete_interrupt(uint32_t dma, uint8_t channel);
bool dma_get_interrupt_flag(uint32_t dma, uint8_t channel, uint32_t flags);
void dma_disable_transfer_complete_interrupt(uint32_t dma, uint8_t channel);
/* the channel's registers: CCR EN, CNDTR, CMAR */
uint8_t dmaEnabled(void);
uint16_t dmaRemaining(void);
uint32_t dmaMemoryAddress(void);

#define NVIC_DMA2_CHANNEL4_5_IRQ 59
void nvic_set_priority(uint8_t irqn, uint8_t priority);
void nvic_enable_irq(uint8_t irqn);

/* BASEPRI and IPSR */
uint32_t maskLog(void);
void unmaskLog(uint32_t old);
uint32_t currentIpsr(void);
uint32_t interruptsDisabled(void);

/* Called before every byte is put into the ring: the test lets the UART run, and runs the DMA interrupt or the
   scheduler's interrupt if the masking at that moment allows it. */
void hostPreemptionPoint(void);
/* The main loop waits for room in the ring: time passes, the UART runs, and interrupts may run. */
void hostWaitForRoom(void);
/* Called for every byte put into the ring, after it is written. */
void hostRecordByte(uint8_t c);

#endif
