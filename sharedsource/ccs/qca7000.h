/* Interface header for qca7000.c */

/* Global Defines */

#define MY_SPI_TX_RX_BUFFER_SIZE 2100
#define MY_ETH_TRANSMIT_BUFFER_LEN 250
#define MY_ETH_RECEIVE_BUFFER_LEN 250

/* Global Variables */

extern uint16_t mySpiDataSize;
extern uint8_t mySpiRxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
extern uint8_t mySpiTxBuffer[MY_SPI_TX_RX_BUFFER_SIZE];
extern uint32_t nTotalEthReceiveBytes; /* total number of bytes which has been received from the ethernet port */
extern uint32_t nTotalTransmittedBytes;
extern uint8_t myethtransmitbuffer[MY_ETH_TRANSMIT_BUFFER_LEN];
extern uint16_t myethtransmitbufferLen; /* The number of used bytes in the ethernet transmit buffer */
extern uint8_t myethreceivebuffer[MY_ETH_RECEIVE_BUFFER_LEN];
extern uint16_t myethreceivebufferLen;
extern const uint8_t myMAC[6];
extern uint8_t nMaxInMyEthernetReceiveCallback, nInMyEthernetReceiveCallback;
extern uint16_t nTcpPacketsReceived;

/* Global Functions */

extern void mySpiTransmitReceive(void);
extern void demoQCA7000(void);
extern void spiQCA7000checkForReceivedData(void);
extern void myEthTransmit(void);

