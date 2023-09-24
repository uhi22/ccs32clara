/* Interface header for myAdc */

/* Global Defines */

#define MYADC_NUMBER_OF_CHANNELS 8 /* 3 temperatures, 1 U_HV, 1 CpuTemp, LockFeedback, Pushbutton, PP */

#define MY_ADC_CHANNEL_TEMP1 0
#define MY_ADC_CHANNEL_TEMP2 1
#define MY_ADC_CHANNEL_TEMP3 2
#define MY_ADC_CHANNEL_DCVOLTAGE 3
#define MY_ADC_CHANNEL_CPUTEMP 4
#define MY_ADC_CHANNEL_LOCKFEEDBACK 5
#define MY_ADC_CHANNEL_PUSHBUTTON 6
#define MY_ADC_CHANNEL_PP 7

/* Global Variables */
extern ADC_HandleTypeDef hadc1;  /* The handle of the ADC1 */
extern ADC_ChannelConfTypeDef sConfig; /* The ADC config structure, used by the generated code (CubeMX) in main.c,
                                          and also used by the own functions for channel selection. */

extern uint16_t rawAdValues[MYADC_NUMBER_OF_CHANNELS];
extern float fCpuTemperature;
extern int16_t uCcsInlet_V;

/* Global Functions */

extern uint16_t myAdc_analogRead(uint8_t channel);
extern void myAdc_demo(void);
extern void myAdc_calibrate(void);
extern void myAdc_cyclic(void);


