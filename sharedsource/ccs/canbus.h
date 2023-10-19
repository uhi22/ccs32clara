/* Interface header for canbus.c */

/* Global Defines */

/* Global Variables */
extern CAN_HandleTypeDef hcan;
extern int16_t canDebugValue1, canDebugValue2, canDebugValue3, canDebugValue4;
extern uint8_t canRxData[8]; /* written in irq context */

/* Global Functions */

extern void canbus_Init(void);
extern void canbus_Mainfunction100ms(void);
extern void canbus_Mainfunction10ms(void);
extern void canbus_Mainfunction1ms(void);
extern void canbus_addStringToTextTransmitBuffer(char *s);
extern void canbus_addToBinaryLogging(uint16_t preamble, uint8_t *inbuffer, uint16_t inbufferLen);
