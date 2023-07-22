/* Interface header for myAdc */

/* Global Defines */


/* Global Variables */
extern ADC_HandleTypeDef hadc1;  /* The handle of the ADC1 */
extern ADC_ChannelConfTypeDef sConfig; /* The ADC config structure, used by the generated code (CubeMX) in main.c,
                                          and also used by the own functions for channel selection. */
/* Global Functions */

extern uint16_t myAdc_analogRead(uint8_t channel);
extern void myAdc_demo(void);


