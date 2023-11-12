
/* QCA7000 / QCA7005 PLC modem driver at STM32.
 github.com/uhi22/....
*/

/* The SPI interface is configured in the CubeIDE, by opening the .ioc file. The Device Configuration Tool
   generates the configuration code into the main.c.
   Here in the driver, we use the SPI via the interface function mySpiTransmitReceive(), which
   is provided by the main.c.

   */


#include "ccs32_globals.h"

uint16_t mySpiDataSize;
uint8_t mySpiRxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
uint8_t mySpiTxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
uint32_t nTotalTransmittedBytes;
uint16_t nTcpPacketsReceived;

uint8_t myethtransmitbuffer[MY_ETH_TRANSMIT_BUFFER_LEN];
uint16_t myethtransmitbufferLen; /* The number of used bytes in the ethernet transmit buffer */
uint8_t myethreceivebuffer[MY_ETH_RECEIVE_BUFFER_LEN];
uint16_t myethreceivebufferLen;

uint16_t debugCounter_cutted_myethreceivebufferLen;

void mySpiTransmitReceive()
{
   DigIo::spics.Clear();
   for (uint32_t i = 0; i < mySpiDataSize; i++) {
      mySpiRxBuffer[i] = spi_xfer(SPI1, mySpiTxBuffer[i]);
   }
   DigIo::spics.Set();
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
}

void spiQCA7000DemoReadSignature(void) {
  /* Demo for reading the signature of the QCA7000. This should show AA55. */
  uint16_t sig;
  uint8_t i;
  i=0;
  mySpiTxBuffer[i++]=0xDA; /* Read, internal, reg 1A (SIGNATURE) */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiDataSize = i;
  mySpiTransmitReceive();
  sig = mySpiRxBuffer[2];
  sig <<= 8;
  sig += mySpiRxBuffer[3];
  printf("Hello, sig is %x\r\n", sig); /* should be AA 55  */
}


void spiQCA7000DemoReadWRBUF_SPC_AVA(void) {
  /* Demo for reading the available write buffer size from the QCA7000 */
  int i;
  i=0;
  mySpiTxBuffer[i++]=0xC2; /*  */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0x00;
  mySpiDataSize = i;
  mySpiTransmitReceive();

  printf("RBUF_SPC_AVA is %x %x\r\n", mySpiRxBuffer[2], mySpiRxBuffer[3]);
}

void spiQCA7000DemoWriteBFR_SIZE(uint16_t n) {
  /* Demo for writing the write buffer size to the QCA7000 */
  int i;
  i=0;
  mySpiTxBuffer[i++]=0x41; /* 0x41 is write, internal, reg 1 */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=n>>8;
  mySpiTxBuffer[i++]=(uint8_t)n;
  mySpiDataSize = i;
  mySpiTransmitReceive();
}


uint16_t spiQCA7000DemoReadRDBUF_BYTE_AVA(void) {
  /* Demo for retrieving the amount of available received data from QCA7000 */
  uint16_t n;
  int i;
  i=0;
  mySpiTxBuffer[i++]=0xC3; /* 0xC3 is read, internal, reg 3 RDBUF_BYTE_AVA */
  mySpiTxBuffer[i++]=0x00;
  mySpiTxBuffer[i++]=0;
  mySpiTxBuffer[i++]=0;
  mySpiDataSize = i;
  mySpiTransmitReceive();

  n = mySpiRxBuffer[2]; /* upper byte of the size */
  n<<=8;
  n+=mySpiRxBuffer[3]; /* lower byte of the size */
  return n;
}


void QCA7000checkRxDataAndDistribute(int16_t availbytes) {
  uint16_t  L1, L2;
  uint8_t *p;
  uint8_t  blDone = 0;
  uint8_t counterOfEthFramesInSpiFrame;
  counterOfEthFramesInSpiFrame = 0;
  p= mySpiRxBuffer;
  while (!blDone) {  /* The SPI receive buffer may contain multiple Ethernet frames. Run through all. */
      /* the SpiRxBuffer contains more than the ethernet frame:
        4 byte length
        4 byte start of frame AA AA AA AA
        2 byte frame length, little endian
        2 byte reserved 00 00
        payload
        2 byte End of frame, 55 55 */
      /* The higher 2 bytes of the len are assumed to be 0. */
      /* The lower two bytes of the "outer" len, big endian: */
      L1 = p[2]; L1<<=8; L1+=p[3];
      /* The "inner" len, little endian. */
      L2 = p[9]; L2<<=8; L2+=p[8];
      if ((p[4]=0xAA) && (p[5]=0xAA) && (p[6]=0xAA) && (p[7]=0xAA)
            && (L2+10==L1)) {
          counterOfEthFramesInSpiFrame++;
          /* The start of frame and the two length informations are plausible. Copy the payload to the eth receive buffer. */
          myethreceivebufferLen = L2;
          /* but limit the length, to avoid buffer overflow */
          if (myethreceivebufferLen > MY_ETH_RECEIVE_BUFFER_LEN) {
              myethreceivebufferLen = MY_ETH_RECEIVE_BUFFER_LEN;
              debugCounter_cutted_myethreceivebufferLen++;
          }
          memcpy(myethreceivebuffer, &p[12], myethreceivebufferLen);
          /* We received an ethernet package. Determine its type, and dispatch it to the related handler. */
          uint16_t etherType = getEtherType(myethreceivebuffer);
          #ifdef VERBOSE_QCA7000
            showAsHex(myethreceivebuffer, myethreceivebufferLen, "eth.myreceivebuffer");
          #endif
          if (etherType == 0x88E1) { /* it is a HomePlug message */
            //Serial.println("Its a HomePlug message.");
            addToTrace("Its a HomePlug message.");
            //canbus_addToBinaryLogging(0xAA5A, myethreceivebuffer, myethreceivebufferLen);
            evaluateReceivedHomeplugPacket();
          } else if (etherType == 0x86dd) { /* it is an IPv6 frame */
           addToTrace("Its a IPv6 message.");
            //canbus_addToBinaryLogging(0xAA5A, myethreceivebuffer, myethreceivebufferLen);
            ipv6_evaluateReceivedPacket();
          } else {
            //Serial.println("Other message.");
            /* We do not log other messages, to avoid too much logging traffic. If needed, enable the logging,
            by commenting-in this: canbus_addToBinaryLogging(0xAA5A, myethreceivebuffer, myethreceivebufferLen);
            */
          }
          availbytes = availbytes - L1 - 4;
          p+= L1+4;
          //Serial.println("Avail after first run:" + String(availbytes));
          if (availbytes>10) { /*
            Serial.println("There is more data.");
            Serial.print(String(p[0], HEX) + " ");
            Serial.print(String(p[1], HEX) + " ");
            Serial.print(String(p[2], HEX) + " ");
            Serial.print(String(p[3], HEX) + " ");
            Serial.print(String(p[4], HEX) + " ");
            Serial.print(String(p[5], HEX) + " ");
            Serial.print(String(p[6], HEX) + " ");
            Serial.print(String(p[7], HEX) + " ");
            Serial.print(String(p[8], HEX) + " ");
            Serial.print(String(p[9], HEX) + " ");
            */
          } else {
            blDone=1;
          }
    } else {
        /* no valid header -> end */
        blDone=1;
    }
  }
  #ifdef VERBOSE_QCA7000
    Serial.println("QCA7000: The SPI frame contained " + String(counterOfEthFramesInSpiFrame) + " ETH frames.");
  #endif
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
  printf("avail rx bytes: %d\r\n", availBytes);
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
      - The next data will be read on the next call. Make sure that data of ONE ethernet frame which
        is splitted over multiple call cycles is correctly treated (not implemented yet).
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
    mySpiTransmitReceive();
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
  mySpiTransmitReceive();
}

/* The Ethernet transmit function. */
void myEthTransmit(void) {
  nTotalTransmittedBytes += myethtransmitbufferLen;
  #ifdef VERBOSE_QCA7000
    showAsHex(myethtransmitbuffer, myethtransmitbufferLen, "myEthTransmit");
  #endif
  //canbus_addToBinaryLogging(0xAA55, myethtransmitbuffer, myethtransmitbufferLen);
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
