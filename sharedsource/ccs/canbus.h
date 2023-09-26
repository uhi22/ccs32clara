/* Interface header for canbus.c */

/* Global Defines */

/* Global Variables */
extern CAN_HandleTypeDef hcan;
extern int16_t canDebugValue1, canDebugValue2, canDebugValue3, canDebugValue4;

/* Global Functions */

extern void canbus_Init(void);
extern void canbus_Mainfunction100ms(void);

