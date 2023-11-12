

/* interface data */
extern uint8_t xcpSlavePdu[8];
extern uint8_t xcp_isTransmitMessageReady;

/* interface functions */
extern void xcp_CanRxInterrupt(void);
extern void xcp_Mainfunction(void);
extern void xcp_Init(void);
