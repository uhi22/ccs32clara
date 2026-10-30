
/* QCA7000 / QCA7005 PLC modem driver at STM32.
 github.com/uhi22/....
*/

/* The SPI interface is configured in the CubeIDE, by opening the .ioc file. The Device Configuration Tool
   generates the configuration code into the main.c.
   Here in the driver, we use the SPI via the interface function mySpiTransmitReceive(), which
   is provided by the main.c.

   */


#ifdef QCA7000_HOST_TEST
#include "qca7000_hosttest.h" /* test/host: the SPI, the DMA and the modem, simulated */
#else
#include "ccs32_globals.h"
#include <libopencm3/cm3/dwt.h>
#include <libopencm3/stm32/rcc.h>
#endif
#include "qcaframe.h"

static uint16_t mySpiDataSize;
static uint8_t mySpiRxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
static uint8_t mySpiTxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
static uint32_t nTotalTransmittedBytes;
uint16_t nTcpPacketsReceived;

uint8_t myethtransmitbuffer[MY_ETH_TRANSMIT_BUFFER_LEN];// = &mySpiTxBuffer[10];
uint16_t myethtransmitbufferLen; /* The number of used bytes in the ethernet transmit buffer */
uint8_t myethreceivebuffer[MY_ETH_RECEIVE_BUFFER_LEN];// = &mySpiRxBuffer[12];
uint16_t myethreceivebufferLen;

uint16_t debugCounter_cutted_myethreceivebufferLen;

/* Only a read of the QCA7000's receive buffer fills mySpiRxBuffer. Register accesses and frame writes put what the
   QCA7000 clocks back elsewhere: a frame is sent (a TCP ACK, for example) from inside
   QCA7000checkRxDataAndDistribute(), while mySpiRxBuffer still holds frames not yet parsed. */
static uint8_t mySpiRegRxBuffer[4]; /* what the QCA7000 answers to a register access */
static uint8_t spiRxDiscard;        /* where the DMA puts what comes back while a frame is written */

/* The SPI's DMA transfers. The longest, MY_SPI_TX_RX_BUFFER_SIZE (1100) bytes at 72 MHz / 16, takes about 2 ms. */
#define SPI_DMA_TIMEOUT_US 5000
static uint8_t spiDmaCycleCounter; /* the DWT cycle counter runs (qca7000setup); otherwise rtc_get_ms() is the clock */
static uint16_t spiDmaTimeouts;    /* SPI DMA transfers that didn't finish in time, since power-on */
static struct QcaFrameRx qcaRx;    /* the receive framing (QCA7000checkRxDataAndDistribute) */

/* A transfer that didn't finish in time: stop it and end it on the bus, so that the next one starts clean. The
   QCA7000 sees the chip select go high, which ends the transfer for it. Nothing in this driver resynchronises or
   resets the QCA7000; the receive framing starts again at the next frame header, and a frame cut short on the way out
   is lost (TCP sends its data again; SLAC and SDP repeat their requests). */
static void spiDmaAbort(void)
{
   dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
   spi_disable_tx_dma(SPI1);
   spi_disable_rx_dma(SPI1);
   dma_disable_channel(DMA1, DMA_CHANNEL2);
   dma_disable_channel(DMA1, DMA_CHANNEL3);
   dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_TCIF);
   while (SPI_SR(SPI1) & SPI_SR_BSY); /* the byte being shifted out, a few us */
   (void)SPI_DR(SPI1); /* a received byte the DMA no longer takes, so that the next register access starts clean */
   (void)SPI_SR(SPI1);
   DigIo::spics.Set();
   spiDmaTimeouts++;
   addToTrace(MOD_QCA, "[QCA] SPI DMA transfer timed out, stopped. Count:", (int16_t)spiDmaTimeouts);
}

/* Waits until the SPI DMA transfer (a frame written, or the receive buffer read) has finished, and returns 1. It is
   called before mySpiTxBuffer is filled for the next transfer (the DMA reads it while the transfer runs), and before
   mySpiRxBuffer is read. The SPI's busy flag alone isn't enough: it drops between two bytes whenever the DMA is late
   with the next one, so a read could be parsed while the DMA was still filling the buffer, and a new transfer could
   start in the middle of one. If the transfer hasn't finished after SPI_DMA_TIMEOUT_US, it is stopped (spiDmaAbort)
   and 0 is returned. The DMA interrupt raises the chip select once a transfer is complete. */
static uint8_t waitSpiDmaDone(void)
{
   uint32_t startCycles = spiDmaCycleCounter ? dwt_read_cycle_counter() : 0;
   uint32_t startMs = rtc_get_ms();
   uint32_t limitCycles = rcc_ahb_frequency / 1000000 * SPI_DMA_TIMEOUT_US;

   while ((DMA_CCR(DMA1, DMA_CHANNEL2) & DMA_CCR_EN) && dma_get_number_of_data(DMA1, DMA_CHANNEL2) != 0) {
      uint8_t late = spiDmaCycleCounter ? dwt_read_cycle_counter() - startCycles > limitCycles
                                        : rtc_get_ms() - startMs > 20; /* (10 ms ticks) */
      if (late) {
         spiDmaAbort();
         return 0;
      }
   }
   while (SPI_SR(SPI1) & SPI_SR_BSY);
   return 1;
}

static void mySpiTransmitReceive()
{
   waitSpiDmaDone();
   DigIo::spics.Clear();
   for (uint32_t i = 0; i < mySpiDataSize; i++) {
      uint8_t b = spi_xfer(SPI1, mySpiTxBuffer[i]);
      if (i < sizeof(mySpiRegRxBuffer))
         mySpiRegRxBuffer[i] = b;
   }
   DigIo::spics.Set();
}

/* intoRxBuffer: 1 for a read of the receive buffer (into mySpiRxBuffer), 0 for a frame write (what comes back is
   discarded). */
static void dmaTransmitReceive(uint8_t intoRxBuffer)
{
   waitSpiDmaDone();
   dma_disable_channel(DMA1, DMA_CHANNEL2);
   dma_disable_channel(DMA1, DMA_CHANNEL3);
   if (intoRxBuffer) {
      dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t)(uintptr_t)mySpiRxBuffer);
      dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
   } else {
      dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t)(uintptr_t)&spiRxDiscard);
      dma_disable_memory_increment_mode(DMA1, DMA_CHANNEL2);
   }
   dma_set_number_of_data(DMA1, DMA_CHANNEL2, mySpiDataSize);
   dma_set_number_of_data(DMA1, DMA_CHANNEL3, mySpiDataSize);
   dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_TCIF);
   dma_clear_interrupt_flags(DMA1, DMA_CHANNEL3, DMA_TCIF);
   dma_enable_channel(DMA1, DMA_CHANNEL2);
   dma_enable_channel(DMA1, DMA_CHANNEL3);
   DigIo::spics.Clear();
   spi_enable_tx_dma(SPI1);
   spi_enable_rx_dma(SPI1);
   dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
}

extern "C" void dma1_channel2_isr()
{
   if (!dma_get_interrupt_flag(DMA1, DMA_CHANNEL2, DMA_TCIF))
      return; /* only the end of the SPI receive transfer */
   dma_clear_interrupt_flags(DMA1, DMA_CHANNEL2, DMA_TCIF);
   while (SPI_SR(SPI1) & SPI_SR_BSY);
   DigIo::spics.Set();
   spi_disable_tx_dma(SPI1);
   spi_disable_rx_dma(SPI1);
   dma_disable_transfer_complete_interrupt(DMA1, DMA_CHANNEL2);
}

void qca7000setup() {
   spi_init_master(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_16, SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1, SPI_CR1_DFF_8BIT, SPI_CR1_MSBFIRST);

   spi_enable_software_slave_management(SPI1);
   spi_set_nss_high(SPI1);
   gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
   gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);
   spi_enable(SPI1);
   DigIo::spics.Set();
   spiDmaCycleCounter = dwt_enable_cycle_counter(); /* the clock for waitSpiDmaDone() */

   dma_channel_reset(DMA1, DMA_CHANNEL3);
   dma_set_read_from_memory(DMA1, DMA_CHANNEL3);
   dma_set_peripheral_address(DMA1, DMA_CHANNEL3, (uint32_t)(uintptr_t)&SPI_DR(SPI1));
   dma_set_peripheral_size(DMA1, DMA_CHANNEL3, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, DMA_CHANNEL3, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL3);
   dma_set_memory_address(DMA1, DMA_CHANNEL3, (uint32_t)(uintptr_t)mySpiTxBuffer);

   dma_channel_reset(DMA1, DMA_CHANNEL2);
   dma_set_peripheral_address(DMA1, DMA_CHANNEL2, (uint32_t)(uintptr_t)&SPI_DR(SPI1));
   dma_set_peripheral_size(DMA1, DMA_CHANNEL2, DMA_CCR_PSIZE_8BIT);
   dma_set_memory_size(DMA1, DMA_CHANNEL2, DMA_CCR_MSIZE_8BIT);
   dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL2);
   dma_set_memory_address(DMA1, DMA_CHANNEL2, (uint32_t)(uintptr_t)mySpiRxBuffer);
}

static void spiQCA7000DemoReadSignature(void) {
  /* Demo for reading the signature of the QCA7000. This should show AA55. */
  uint16_t sig;
  uint8_t i;
  waitSpiDmaDone(); /* mySpiTxBuffer may still be going out */
  i=0;
  mySpiTxBuffer[i++]=0xDA; /* Read, internal, reg 1A (SIGNATURE) */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiDataSize = i;
  mySpiTransmitReceive();
  sig = mySpiRegRxBuffer[2];
  sig <<= 8;
  sig += mySpiRegRxBuffer[3];
  printf("Hello, sig is %x\r\n", sig); /* should be AA 55  */
}

static void spiQCA7000DemoWriteBFR_SIZE(uint16_t n) {
  /* Demo for writing the write buffer size to the QCA7000 */
  int i;
  waitSpiDmaDone(); /* mySpiTxBuffer may still be going out */
  i=0;
  mySpiTxBuffer[i++]=0x41; /* 0x41 is write, internal, reg 1 */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=n>>8;
  mySpiTxBuffer[i++]=(uint8_t)n;
  mySpiDataSize = i;
  mySpiTransmitReceive();
}


static uint16_t spiQCA7000DemoReadRDBUF_BYTE_AVA(void) {
  /* Demo for retrieving the amount of available received data from QCA7000 */
  uint16_t n;
  int i;
  waitSpiDmaDone(); /* mySpiTxBuffer may still be going out */
  i=0;
  mySpiTxBuffer[i++]=0xC3; /* 0xC3 is read, internal, reg 3 RDBUF_BYTE_AVA */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0;
  mySpiTxBuffer[i++]=0;
  mySpiDataSize = i;
  mySpiTransmitReceive();

  n = mySpiRegRxBuffer[2]; /* upper byte of the size */
  n<<=8;
  n+=mySpiRegRxBuffer[3]; /* lower byte of the size */
  return n;
}


static void distributeEthFrame(void) {
    /* We received an ethernet package. Determine its type, and dispatch it to the related handler. */
    uint16_t etherType = getEtherType(myethreceivebuffer);
    #ifdef VERBOSE_QCA7000
      showAsHex(myethreceivebuffer, myethreceivebufferLen, "eth.myreceivebuffer");
    #endif
    if (etherType == 0x88E1) { /* it is a HomePlug message */
      //Serial.println("Its a HomePlug message.");
      addToTrace(MOD_QCA, "Its a HomePlug message.");
      //canbus_addToBinaryLogging(0xAA5A, myethreceivebuffer, myethreceivebufferLen);
      /* we log the ethernet traffic, but we stop logging it, when we entered the
         charging loop, to avoid spamming the log */
      if (checkpointNumber<700) {
        addToTrace(MOD_ETHTRAFFIC, "ETH rx HP:", myethreceivebuffer, myethreceivebufferLen);
      } else {
        /* We are in the charging loop or beyond. Do not log the ethernet traffic. */
      }
      evaluateReceivedHomeplugPacket();
    } else if (etherType == 0x86dd) { /* it is an IPv6 frame */
      addToTrace(MOD_QCA, "Its a IPv6 message.");
      //canbus_addToBinaryLogging(0xAA5A, myethreceivebuffer, myethreceivebufferLen);
      /* we log the ethernet traffic, but we stop logging it, when we entered the
         charging loop, to avoid spamming the log */
      if (checkpointNumber<700) {
        addToTrace(MOD_ETHTRAFFIC, "ETH rx IP:", myethreceivebuffer, myethreceivebufferLen);
      } else {
        /* We are in the charging loop or beyond. Do not log the ethernet traffic. */
      }
      ipv6_evaluateReceivedPacket();
    } else {
      //Serial.println("Other message.");
      /* We do not log other messages, to avoid too much logging traffic. If needed, enable the logging,
      by commenting-in this: addToTrace(MOD_ETHTRAFFIC, "ETH rx other:", myethreceivebuffer, myethreceivebufferLen);
      */
    }
}

void QCA7000checkRxDataAndDistribute(int16_t availbytes) {
  /* The SPI receive buffer holds what we read from the QCA7000: a number of frames, each with the QCA framing around
     it (see qcaframe.h). The read is limited to our buffer size, so the last frame may be cut, and its end comes with
     the next read. qcaFrameRx_take() keeps the part until then, and gives us only complete frames. */
  uint16_t used = 0, frameLen;
  if (qcaRx.buf == 0) {
    qcaFrameRx_init(&qcaRx, myethreceivebuffer, MY_ETH_RECEIVE_BUFFER_LEN);
  }
  while (availbytes > 0 && used < (uint16_t)availbytes) {
    used += qcaFrameRx_take(&qcaRx, &mySpiRxBuffer[used], (uint16_t)availbytes - used, &frameLen);
    if (frameLen > 0) {
      myethreceivebufferLen = frameLen;
      distributeEthFrame();
    }
  }
  debugCounter_cutted_myethreceivebufferLen = qcaRx.nDropped; /* frames dropped: too large, too small or out of step */
}


void spiQCA7000checkForReceivedData(void) {
  /* checks whether the QCA7000 indicates received data, and if yes, fetches the data. */
  uint16_t availBytes;
  uint16_t i;
  i=0;
  availBytes = spiQCA7000DemoReadRDBUF_BYTE_AVA();
  if (availBytes==0) {
     return; /* nothing to do */
  }
  //printf("avail rx bytes: %d\r\n", availBytes);
  if (availBytes<4000) {
      /* in case we would see more then 4000 bytes, this is most likely an error in SPI transmission. We ignore this and in the next
        loop maybe we read a better value */
    //#ifdef VERBOSE_QCA7000
      //Serial.println("QCA7000 avail RX bytes: " + String(availBytes));
    //#endif
    /* If the QCA indicates that the receive buffer contains data, the following procedure
    is necessary to get the data (according to https://chargebyte.com/assets/Downloads/an4_rev5.pdf)
       - write the BFR SIZE, this sets the length of data to be read via external read
       - start an external read and receive as much data as set in SPI REG BFR SIZE before */

    /* For the case that the QCA contains more data than we could hold in our SPI buffer, we limit
    the size here. This may/will cause data loss, which could be handled by the following strategies:
      - The next data will be read on the next call. Data of ONE ethernet frame which is split over
        two reads is put together by QCA7000checkRxDataAndDistribute().
      - Rely on the retry on the higher layers, and on the fact that nobody will send so much data
        in the given project. Ok as starting point, but can be improved. */
    if (availBytes+2 >= MY_SPI_TX_RX_BUFFER_SIZE) {
        availBytes = MY_SPI_TX_RX_BUFFER_SIZE - 2;
    }
    /* announce to the QCA how many bytes we want to fetch */
    spiQCA7000DemoWriteBFR_SIZE(availBytes);

    mySpiTxBuffer[i++]=0x80; /* 0x80 is read, external */
    mySpiTxBuffer[i++]=0x00;

    mySpiDataSize = availBytes+2;
    //mySpiTransmitReceive();
    dmaTransmitReceive(1);
    if (!waitSpiDmaDone()) {
       /* The read didn't finish: what is in mySpiRxBuffer is incomplete, and a part of the QCA7000's data is lost.
          Don't parse it, and drop a frame begun in an earlier read: the framing starts again at the next header. */
       qcaFrameRx_init(&qcaRx, myethreceivebuffer, MY_ETH_RECEIVE_BUFFER_LEN);
       return;
    }
    /* discard the first two bytes, they do not belong to the data */
    for (i=0; i<availBytes; i++) {
       mySpiRxBuffer[i] = mySpiRxBuffer[i+2];
    }
    QCA7000checkRxDataAndDistribute(availBytes); /* Takes the data from the SPI rx buffer, splits it into ethernet frames and distributes them. */
  }
}

void spiQCA7000SendEthFrame(void) {
  /* to send an ETH frame, we need two steps:
     1. Write the BFR_SIZE (internal reg 1)
     2. Write external, preamble, size, data */
/* Example (from CCM)
  The SlacParamReq has 60 "bytes on wire" (in the Ethernet terminology).
  The   BFR_SIZE is set to 0x46 (command 41 00 00 46). This is 70 decimal.
  The transmit command on SPI is
  00 00
  AA AA AA AA
  3C 00 00 00    (where 3C is 60, matches the "bytes on wire")
  <60 bytes payload>
  55 55 After the footer, the frame is finished according to the qca linux driver implementation.
  xx yy But the Hyundai CCM sends two bytes more, either 00 00 or FE 80 or E1 FF or other. Most likely not relevant.
  Protocol explanation from https://chargebyte.com/assets/Downloads/an4_rev5.pdf
*/
  /* Todo:
    1. Check whether the available transmit buffer size is big enough to get the intended frame.
       If not, this is an error situation, and we need to instruct the QCA to heal, e.g. by resetting it.
  */
  spiQCA7000DemoWriteBFR_SIZE(myethtransmitbufferLen+10); /* The size in the BFR_SIZE is 10 bytes more than in the size after the preamble below (in the original CCM trace) */
  mySpiTxBuffer[0] = 0x00; /* external write command */
  mySpiTxBuffer[1] = 0x00;
  mySpiTxBuffer[2] = 0xAA; /* Start of frame */
  mySpiTxBuffer[3] = 0xAA;
  mySpiTxBuffer[4] = 0xAA;
  mySpiTxBuffer[5] = 0xAA;
  mySpiTxBuffer[6] = (uint8_t)myethtransmitbufferLen; /* LSB of the length */
  mySpiTxBuffer[7] = myethtransmitbufferLen>>8; /* MSB of the length */
  mySpiTxBuffer[8] = 0x00; /* to bytes reserved, 0x00 */
  mySpiTxBuffer[9] = 0x00;
  memcpy(&mySpiTxBuffer[10], myethtransmitbuffer, myethtransmitbufferLen); /* the ethernet frame */
  mySpiTxBuffer[10+myethtransmitbufferLen] = 0x55; /* End of frame, 2 bytes with 0x55. Aka QcaFrmCreateFooter in the linux driver */
  mySpiTxBuffer[11+myethtransmitbufferLen] = 0x55;
  mySpiDataSize = 12+myethtransmitbufferLen;
  //mySpiTransmitReceive();
  dmaTransmitReceive(0); /* what comes back is discarded: mySpiRxBuffer may hold frames not yet parsed */
}

/* The Ethernet transmit function. */
void myEthTransmit(void) {
  nTotalTransmittedBytes += myethtransmitbufferLen;
  #ifdef VERBOSE_QCA7000
    showAsHex(myethtransmitbuffer, myethtransmitbufferLen, "myEthTransmit");
  #endif
  //canbus_addToBinaryLogging(0xAA55, myethtransmitbuffer, myethtransmitbufferLen);
  /* we log the ethernet traffic, but we stop logging it, when we entered the charging loop, to avoid
  spamming the log */
  if (checkpointNumber<700) {
      addToTrace(MOD_ETHTRAFFIC, "ETH will transmit:", myethtransmitbuffer, myethtransmitbufferLen);
  } else {
      /* We are in the charging loop or beyond. Do not log the ethernet traffic. */
  }
  spiQCA7000SendEthFrame();
}


void demoQCA7000SendSoftwareVersionRequest(void) {
  composeGetSwReq();
  spiQCA7000SendEthFrame();
}


void demoQCA7000(void) {
  spiQCA7000DemoReadSignature();
  //spiQCA7000DemoReadWRBUF_SPC_AVA();
  demoQCA7000SendSoftwareVersionRequest();
  spiQCA7000checkForReceivedData();
  spiQCA7000checkForReceivedData();
}
