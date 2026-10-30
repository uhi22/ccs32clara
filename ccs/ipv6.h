/* Interface header for ipv6.c */

/* Global Defines */

/* Global Variables */

extern uint16_t evccPort;
extern uint16_t seccTcpPort; /* the port number of the charger */
extern const uint8_t EvccIp[16];
extern uint8_t SeccIp[16];

/* Global Functions */

extern void ipv6_initiateSdpRequest(void);
extern uint8_t ipv6_isChargerTlsOnly(void); /* the charger answered SDP with TLS, and only ISO 15118-2 is offered */
extern void ipv6_clearChargerTlsOnly(void);
extern void ipv6_evaluateReceivedPacket(void);


