/* Host test (test/host/qca7000_test.cpp): what ccs/qca7000.cpp uses from the hardware and the rest of Clara, as a
   simulation that the test drives. Built with -DQCA7000_HOST_TEST; nothing here is in the firmware. */

#ifndef QCA7000_HOSTTEST_H
#define QCA7000_HOSTTEST_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "qca7000.h"

/* the rest of Clara */
#define addToTrace(...)
#define MOD_QCA 0
#define MOD_ETHTRAFFIC 0
extern uint16_t checkpointNumber;
uint16_t getEtherType(uint8_t* messagebufferbytearray);
void evaluateReceivedHomeplugPacket(void);
void ipv6_evaluateReceivedPacket(void);
void composeGetSwReq(void);

/* the chip select */
struct HostPin
{
   void Set(void);
   void Clear(void);
};
namespace DigIo
{
extern HostPin spics;
}

/* SPI1 */
#define SPI1 1u
#define SPI_SR_BSY 0x80u
#define SPI_SR(spi) hostSpiSr()
#define SPI_DR(spi) hostSpiDr
#define SPI_CR1_BAUDRATE_FPCLK_DIV_16 0
#define SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE 0
#define SPI_CR1_CPHA_CLK_TRANSITION_1 0
#define SPI_CR1_DFF_8BIT 0
#define SPI_CR1_MSBFIRST 0
extern uint32_t hostSpiDr;
uint32_t hostSpiSr(void);
uint8_t spi_xfer(uint32_t spi, uint8_t data);
static inline void spi_init_master(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}
static inline void spi_enable_software_slave_management(uint32_t) {}
static inline void spi_set_nss_high(uint32_t) {}
static inline void spi_enable(uint32_t) {}
void spi_enable_tx_dma(uint32_t spi);   /* the DMA transfer starts */
static inline void spi_enable_rx_dma(uint32_t) {}
static inline void spi_disable_tx_dma(uint32_t) {}
static inline void spi_disable_rx_dma(uint32_t) {}

/* GPIO */
#define GPIOA 0u
#define GPIO5 0u
#define GPIO6 0u
#define GPIO7 0u
#define GPIO_MODE_OUTPUT_50_MHZ 0u
#define GPIO_MODE_INPUT 0u
#define GPIO_CNF_OUTPUT_ALTFN_PUSHPULL 0u
#define GPIO_CNF_INPUT_FLOAT 0u
static inline void gpio_set_mode(uint32_t, uint32_t, uint32_t, uint32_t) {}

/* DMA1 channel 2 (SPI receive) and 3 (SPI transmit) */
#define DMA1 1u
#define DMA_CHANNEL2 2u
#define DMA_CHANNEL3 3u
#define DMA_TCIF 2u
#define DMA_CCR_EN 1u
#define DMA_CCR_PSIZE_8BIT 0u
#define DMA_CCR_MSIZE_8BIT 0u
#define DMA_CCR(dma, channel) hostDmaCcr(channel)
uint32_t hostDmaCcr(uint8_t channel);
void dma_channel_reset(uint32_t dma, uint8_t channel);
void dma_set_read_from_memory(uint32_t dma, uint8_t channel);
static inline void dma_set_peripheral_address(uint32_t, uint8_t, uint32_t) {}
static inline void dma_set_peripheral_size(uint32_t, uint8_t, uint32_t) {}
static inline void dma_set_memory_size(uint32_t, uint8_t, uint32_t) {}
void dma_enable_memory_increment_mode(uint32_t dma, uint8_t channel);
void dma_disable_memory_increment_mode(uint32_t dma, uint8_t channel);
void dma_set_memory_address(uint32_t dma, uint8_t channel, uint32_t address);
void dma_set_number_of_data(uint32_t dma, uint8_t channel, uint16_t number);
uint16_t dma_get_number_of_data(uint32_t dma, uint8_t channel);
void dma_enable_channel(uint32_t dma, uint8_t channel);
void dma_disable_channel(uint32_t dma, uint8_t channel);
bool dma_get_interrupt_flag(uint32_t dma, uint8_t channel, uint32_t flags);
void dma_clear_interrupt_flags(uint32_t dma, uint8_t channel, uint32_t flags);
void dma_enable_transfer_complete_interrupt(uint32_t dma, uint8_t channel);
void dma_disable_transfer_complete_interrupt(uint32_t dma, uint8_t channel);

/* the clocks: the DWT cycle counter, the core clock, and the 10 ms RTC */
bool dwt_enable_cycle_counter(void);
uint32_t dwt_read_cycle_counter(void);
extern uint32_t rcc_ahb_frequency;
uint32_t rtc_get_ms(void);

#endif
