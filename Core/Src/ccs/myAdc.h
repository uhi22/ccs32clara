/* Interface header for myAdc */

/* Global Defines */

#define MYADC_NUMBER_OF_CHANNELS 5 /* 3 temperatures, 1 U_HV, 1 CpuTemp */

/* Global Variables */
extern ADC_HandleTypeDef hadc1;  /* The handle of the ADC1 */
extern ADC_ChannelConfTypeDef sConfig; /* The ADC config structure, used by the generated code (CubeMX) in main.c,
                                          and also used by the own functions for channel selection. */

extern uint16_t rawAdValues[MYADC_NUMBER_OF_CHANNELS];
extern float fCpuTemperature;

/* Global Functions */

extern uint16_t myAdc_analogRead(uint8_t channel);
extern void myAdc_demo(void);
extern void myAdc_cyclic(void);


