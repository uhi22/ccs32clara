
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

static float ohmToCelsiusNTC(float rNTC) {
    /* Convert the resistance to a temperature */
    /* Based on: https://learn.adafruit.com/thermistor/using-a-thermistor */
    float steinhart;
    steinhart = rNTC / Param::GetFloat(Param::TempSensorNomRes);     // (R/Ro)
    steinhart = log(steinhart);                  // ln(R/Ro)
    steinhart /= Param::GetFloat(Param::TempSensorBeta); // 1/B * ln(R/Ro)
    steinhart += 1.0f / (TEMPERATURENOMINAL + 273.15f); // + (1/To)
    steinhart = 1.0f / steinhart;                 // Invert
    steinhart -= 273.15f;                         // convert to C
    return steinhart;
}

/**
 * Converts PT1000 resistance (in ohms) to temperature (in Celsius)
 * Uses linear approximation: R = R0 * (1 + alpha * T)
 * where R0 = 1000Ω at 0°C, alpha ≈ 0.00385 Ω/Ω/°C
 * 
 * Valid range: approximately -200°C to +850°C
 * Precision: ±1K
 * 
 * @param resistance Resistance value in ohms
 * @return Temperature in degrees Celsius
 */
static float ohmToCelsiusPT1000(float resistance) {
    const float R0 = 1000.0;      // Resistance at 0°C (ohms)
    const float ALPHA = 0.00385;  // Temperature coefficient (Ω/Ω/°C)
    
    if (resistance>2000) { /* 2000 ohms are ~270°C. We treat everything above as non-plausible value. */
        return -50.0; /* ... and show -50°C as error value */
    }
    if (resistance<800) { /* 800 ohms are ~-50°C. We treat everything below as non-plausible value. */
        return -50.0; /* ... and show -50°C as error value */
    }
    
    // Linear approximation: T = (R - R0) / (R0 * alpha)
    float temperature = (resistance - R0) / (R0 * ALPHA);
    
    return temperature;
}

static float ohmToCelsius(float resistance_ohm) {
    switch(Param::GetInt(Param::TempSensorType)) {
        case 0:
            return ohmToCelsiusNTC(resistance_ohm);
        case 1:
            return ohmToCelsiusPT1000(resistance_ohm);
        default:
            return -99.0; /* invalid sensor type -> invalid temperature */
    }
}

void temperatures_calculateTemperatures(void) {
    float temp = AnaIn::temp1.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp1, temp);
    float tempMax = temp;

    temp = AnaIn::temp2.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp2, temp);
    tempMax = MAX(tempMax, temp);

    temp = AnaIn::temp3.Get();
    temp = SERIESRESISTOR / ((MAX_ADC_VALUE / temp) - 1.0f);
    temp = ohmToCelsius(temp);
    Param::SetFloat(Param::temp3, temp);
    tempMax = MAX(tempMax, temp);
    Param::SetFloat(Param::MaxTemp, tempMax);

    /* Calculate the pin-temperature-dependent derating of the charge current.
       Goal: Prevent overheating of the inlet and cables.
       Strategy: Three cases.
         1. If the maximum of the three sensors is below the configured threshold,
            the allowed charge current shall be proportional to the gap.
            At the moment hardcoded AMPS_PER_KELVIN = 5, this means with a gap of 20K we allow 100A.
         2. If the maximum of the three temperature sensors reaches the
            parametrized MaxAllowedPinTemperature, the allowed charge current
            shall reach nearly zero (let's say allow just 2A).
         3. If nothing helps and the temperature further increases, terminate the
            session ("emergency stop") */
    float diffTemp_K = tempMax - Param::GetFloat(Param::MaxPinTemperature); /* good case is negative diffTemp */
    float maxAllowedCurrent_A = Param::GetFloat(Param::ChargeCurrent);
    #define MINIMUM_SENSEFUL_CURRENT_A 2 /* charging below this amperage makes no sense */
    if (diffTemp_K > 10) {
        /* very high temperature (much above the configured limit) --> stop charging completely */
        maxAllowedCurrent_A = 0; /* this will stop the session, evaluated in hardwareInterface_stopChargeRequested() */
    } else if (diffTemp_K >= 0) {
        /* temperature limit is reached. Try to stabilize, using a minimum charging current. */
        maxAllowedCurrent_A = MINIMUM_SENSEFUL_CURRENT_A;
    } else {
        /* normal temperature, linear derating 10K below maximum temperature */
        float limit = -diffTemp_K * maxAllowedCurrent_A / 10;
        /* but not below the minimum current: */
        maxAllowedCurrent_A = MIN(maxAllowedCurrent_A, limit);
        maxAllowedCurrent_A = MAX(maxAllowedCurrent_A, 2);
    }
    Param::SetFloat(Param::TempLimitedCurrent, maxAllowedCurrent_A);
}

