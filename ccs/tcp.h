/* Interface header for tcp.c */

/* Global Defines */

/* One received V2GTP message: the 8-byte header and up to 1024 bytes of EXI. The largest response of a DC session
   with external payment is ChargeParameterDiscoveryRes with a SAScheduleList, or ServiceDiscoveryRes with a list of
   services; with the arrays of the EXI codecs filled up, both stay below 1 kB of EXI. */
#define TCP_RX_DATA_LEN (8 + 1024)
#define TCP_PAYLOAD_LEN 200

/* Global Variables */

extern uint16_t tcp_rxdataLen; /* length of a complete message in tcp_rxdata, 0 if none */
extern uint8_t tcp_rxdata[TCP_RX_DATA_LEN];
extern uint8_t tcpPayloadLen;
extern uint8_t* tcpPayload;

/* Global Functions */
#ifdef __cplusplus
extern "C" {
#endif

extern void tcp_Mainfunction(void);
extern void evaluateTcpPacket(void);
extern uint32_t tcp_getTotalNumberOfRetries(void);
/* 1 once after the receive path stopped on an invalid V2GTP header or a message larger than tcp_rxdata; the state
   machine then stops the session. Nothing more is received or acknowledged until the next connection. */
extern uint8_t tcp_takeReceiveError(void);
extern void tcp_Disconnect(void);
extern void tcp_transmit(void);

#ifdef __cplusplus
}
#endif
