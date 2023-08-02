
/* temperatures  */

/* This module handles the temperature sensors.

*/

#include "ccs32_globals.h"
#include <math.h> /* for log() */

/* Example temperature sensors from
  https://www.reichelt.de/ntc-widerstand-0-5-w-10-kohm-ntc-0-2-10k-p13553.html?&trstct=pos_15&nbc=1
  https://www.reichelt.de/index.html?ACTION=7&LA=3&OPEN=0&INDEX=0&FILENAME=A300%2FNTCLE100E-SERIES_ENG_TDS.pdf
  VISHAY NTCLE100E3 with 10k @ 25°C
*/

/* Parameters for temperature calculation */
/* resistance at 25 degrees C */
#define THERMISTORNOMINAL 10000      
/* temperature for nominal resistance (almost always 25 C) */
#define TEMPERATURENOMINAL 25   
/* The beta coefficient of the thermistor (usually 3000-4000) */
/* Value was experimentally evaluated, so that the calculated temperature fits
   to the table in the data sheet. Deviation is <0.5K in the range -10°C to 60°C */
#define BCOEFFICIENT 3900
/* the value of the 'other' resistor */
#define SERIESRESISTOR 10000 /* Nominal 10k Pull-up for the NTC. Different value due to
                     calibration. Smaller numbers lead to a bigger displayed temperature. */

#define MAX_ADC_VALUE 4095 /* we have 12 bit ADC resolution */

float temperatureChannel_1_R_NTC;
float temperatureChannel_1_celsius; 
uint8_t temperatureChannel_1_M40;

float temperatureChannel_2_R_NTC;
float temperatureChannel_2_celsius; 
uint8_t temperatureChannel_2_M40;

float temperatureChannel_3_R_NTC;
float temperatureChannel_3_celsius;
uint8_t temperatureChannel_3_M40;

float fCpuTemperature_Celsius;
uint8_t temperatureCpu_M40;

float ohmToCelsius(float rNTC) {
    /* Convert the resistance to a temperature */
    /* Based on: https://learn.adafruit.com/thermistor/using-a-thermistor */
    float steinhart;
    steinhart = rNTC / THERMISTORNOMINAL;     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
    steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
    steinhart = 1.0 / steinhart;                 // Invert
    steinhart -= 273.15;                         // convert to C
    return steinhart;
}

uint8_t temperatures_convertFloatToM40(float t_celsius) {
	/* converts a float temperature (in celsius) into an uint8 which has
	 * the offset=-40°C and LSB=1Kelvin.
	 */
	int16_t intermediate;
	intermediate = (t_celsius+40);
    if (intermediate<0) intermediate=0; /* saturate at value 0 == -40°C */
    if (intermediate>255) intermediate=255; /* saturate at value 255 == 215°C */
    return (uint8_t)intermediate;
}

void temperatures_calculateTemperatures(void) {
    uint32_t tmp32;
    tmp32 = rawAdValues[0];
    temperatureChannel_1_R_NTC = SERIESRESISTOR / (((float)MAX_ADC_VALUE / (float)tmp32)  - (float)1);
    temperatureChannel_1_celsius = ohmToCelsius(temperatureChannel_1_R_NTC);
    temperatureChannel_1_M40 = temperatures_convertFloatToM40(temperatureChannel_1_celsius);

    tmp32 = rawAdValues[1];
    temperatureChannel_2_R_NTC = SERIESRESISTOR / (((float)MAX_ADC_VALUE / (float)tmp32)  - (float)1);
    temperatureChannel_2_celsius = ohmToCelsius(temperatureChannel_2_R_NTC);
    temperatureChannel_2_M40 = temperatures_convertFloatToM40(temperatureChannel_2_celsius);

    tmp32 = rawAdValues[2];
    temperatureChannel_3_R_NTC = SERIESRESISTOR / (((float)MAX_ADC_VALUE / (float)tmp32)  - (float)1);
    temperatureChannel_3_celsius = ohmToCelsius(temperatureChannel_3_R_NTC);
    temperatureChannel_3_M40 = temperatures_convertFloatToM40(temperatureChannel_3_celsius);

    temperatureCpu_M40 =  temperatures_convertFloatToM40(fCpuTemperature_Celsius);

#ifdef TRACE_THE_TEMPERATURES
    sprintf(strTmp, "NTC1 %4.1f ohm %4.1f celsius", temperatureChannel_1_R_NTC, temperatureChannel_1_celsius);
    addToTrace(strTmp);
    sprintf(strTmp, "NTC2 %4.1f ohm %4.1f celsius", temperatureChannel_2_R_NTC, temperatureChannel_2_celsius);
    addToTrace(strTmp);
    sprintf(strTmp, "NTC3 %4.1f ohm %4.1f celsius", temperatureChannel_3_R_NTC, temperatureChannel_3_celsius);
    addToTrace(strTmp);
#endif
}

