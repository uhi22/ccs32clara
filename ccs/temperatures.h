/* Interface header for temperatures */

/* Global Defines */


/* Global Variables */
extern float temperatureChannel_1_R_NTC;
extern float temperatureChannel_1_celsius;
extern uint8_t temperatureChannel_1_M40;

extern float temperatureChannel_2_R_NTC;
extern float temperatureChannel_2_celsius; 
extern uint8_t temperatureChannel_2_M40;

extern float temperatureChannel_3_R_NTC;
extern float temperatureChannel_3_celsius;
extern uint8_t temperatureChannel_3_M40;

extern float fCpuTemperature_Celsius;
extern uint8_t temperatureCpu_M40;

/* Global Functions */

extern void temperatures_calculateTemperatures(void);



