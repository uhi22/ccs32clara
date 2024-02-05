
/* proximitypilot  */

/* This module handles the PP contact.
   The PP pin is used to inform the car about the following two things:
      1. Whether a plug is present in the vehicles inlet. That's why an other name of PP is "Plug Present".
      2. What is the current limitation of the cable. Different resistor values between PP and PE showing different
         current capacities. This feature is only relevant for AC charging.
*/

#include "ccs32_globals.h"
#include <math.h> /* for log() */


#define MAX_ADC_VALUE 4095 /* we have 12 bit ADC resolution */


void pp_evaluateProximityPilot(void) {
    float temp = AnaIn::pp.Get();
    float U_refAdc, U_pull, U_meas, Rv, R;
    float iLimit;
    
    /* Step 1: Provide the raw AD value (0 to 4095) for analysis purposes. */
    Param::SetFloat(Param::adcProximityPilot, temp);
    
    /* Step 2: Calculate the PP resistance */
    /* Todo: consider the input circuit variant:
         - old Foccci v4.1 has x ohm pull-up to 3V3 and no pull-down
         - next Foccci v4.2 has 330 ohm pull-up to 5V and a 3k pull-down and a 47k by 47k divider to match the 3V3 ADC input.
         - there could be an additional pull-down in the CCS inlet.
       The parameter ppvariant shall select which hardware circuit is installed. */
       
    if (Param::GetInt(Param::ppvariant) == 0) {
        /* The old Foccci (until revision 4.1) */
        U_refAdc = 3.3; /* volt. The full-scale voltage of the ADC */
        U_pull = 3.3; /* volt. The effective pull-up-voltage, created by pull-up and pull-down resistor. In this case, no pull-down at all. */
        U_meas = U_refAdc * temp/4095; /* The measured voltage on the PP. In this case no divider. */
        Rv = 1000.0; /* The effective pull-up resistor. In this case 1k. */
        /* Todo: verify on hardware */
    } else {
        /* The newer Foccci with 330 ohm pull-up to 5V and a 3k pull-down and a 47k by 47k divider */
        U_refAdc = 3.3; /* volt. The full-scale voltage of the ADC */
        U_pull = 4.5; /* volt. The effective pull-up-voltage, created by pull-up and pull-down resistor. */
        U_meas = U_refAdc * temp/4095 * (47.0+47.0)/47.0; /* The measured voltage on the PP. In this case the ADC gets half the PP voltage. */
        Rv = 1 / (1/330.0 + 1/3000.0); /* The effective pull-up resistor. In this case 330ohm parallel 3000ohm. */
        /* tested on hand-wired Foccci. Calculation is ok. */
    }
    if (U_meas>(U_pull-0.5)) {
        /* The voltage on PP is quite high. We do not calculate the R, because this may lead to overflow. Just
           give a high resistance value. */
        R = 10000.0; /* ohms */
    } else {
        /* calculate the resistance of the PP */
        R = Rv / (U_pull/U_meas-1);
    }
    Param::SetFloat(Param::resistanceProximityPilot, R);
    
    /* Step 3: Calculate the cable current limit */
    /* Reference: https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/ */
    iLimit = 0;
    if ((R>1500*0.8) && (R<1500*1.2)) { iLimit = 13; /* 1k5 is 13A, or digital communication */ }
    if ((R>680*0.8) &&  (R<680*1.2)) { iLimit = 20; /* 680ohm is 20A */ }
    if ((R>220*0.8) && (R<220*1.2)) { iLimit = 32; /* 220ohm is 32A */ }
    if ((R>100*0.8) && (R<100*1.2)) { iLimit = 63; /* 100ohm is 63A */ }
    Param::SetFloat(Param::cableCurrentLimit, iLimit);

}

