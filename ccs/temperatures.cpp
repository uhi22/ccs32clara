
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

static float ohmToCelsius(float rNTC) {
    /* Convert the resistance to a temperature */
    /* Based on: https://learn.adafruit.com/thermistor/using-a-thermistor */
    float steinhart;
    steinhart = rNTC / Param::GetFloat(Param::tmpsnsnom);     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= Param::GetFloat(Param::tmpsnscoeff); // 1/B * ln(R/Ro)
    steinhart += 1.0f / (TEMPERATURENOMINAL + 273.15f); // + (1/To)
    steinhart = 1.0f / steinhart;                 // Invert
    steinhart -= 273.15f;                         // convert to C
    return steinhart;
}

void temperatures_calculateTemperatures(void) {
    float temp = AnaIn::temp1.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp1, temp);

    temp = AnaIn::temp2.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp2, temp);

    temp = AnaIn::temp3.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp3, temp);
}

