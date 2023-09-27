/* Interface header for pushbutton.c */

/* Global Defines */

/* Global Variables */
 
extern uint16_t pushbutton_accumulatedButtonDigits; /* four-digit-value which was entered with the pushbutton */
extern uint16_t pushbutton_tButtonReleaseTime;
extern uint16_t pushbutton_tButtonPressTime;
extern uint8_t pushbutton_nNumberOfButtonPresses;
extern uint8_t pushbutton_buttonSeriesCounter;

#define PUSHBUTTON_CYCLES_PER_SECOND 33 /* 30ms call cycle means 33 runs per second */


/* Global Functions */

extern void pushbutton_handlePushbutton(void);

