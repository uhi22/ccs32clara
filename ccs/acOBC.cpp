
/* AC Onboard charger handling  */

/* This module handles the communication to an onboard AC charger.
   Reception and decoding of the status message of the OBC, to use it for StateC, LEDs and connector locking.
*/

#include "ccs32_globals.h"

static uint8_t isBasicAcCharging = 0; /* shows whether we are connected to a basic AC charger (CP PWM 8% to 96%) */
static uint8_t stateBasicAcCharging = 0; /* invalid */

/* From the AC Onboard Charger, we receive a status signal, which tells us, what to do with the connector lock,
   LEDs and StateC. We take over the meaning of ST_CHGNG as far as possible from here:
   https://openinverter.org/forum/viewtopic.php?p=66713#p66713
   and define
     0 invalid. Clara ignores the data.
     1 initializing. Clara locks the connector. Green light flashing.
     2 charging. Clara activates the 1k3 resistor (StateC). LED blue.
     3 (pause. Clara ignores this value)
     4 charging finished. Clara deactivates the 1k3 resistor. Todo: also unlocking the connector? LED green/blue flashing.
     5 charge error. LED red. Todo: Unlocking the connector?
     >=6 invalid. Clara ignores the data.
*/

#define ACOBC_STATE_INITIALIZING 1
#define ACOBC_STATE_CHARGING 2
#define ACOBC_STATE_CHARGEFINISHED 4
#define ACOBC_STATE_CHARGEERROR 5

static uint8_t debounceCounterUntilAcValid = 0;

void acOBC_activateBasicAcCharging(void) {
  /* This function shall be called if we detect a CP PWM in the "analog range". */
  isBasicAcCharging = 1;
}

void acOBC_deactivateBasicAcCharging(void) {
  /* This function shall be called whenever there is an hint that we are connected to something else that an analog AC charger. */
  isBasicAcCharging = 0;
}

uint8_t acOBC_isBasicAcCharging(void) {
    return isBasicAcCharging;
}

uint8_t acOBC_getRGB(void) {
    static uint8_t cycleDivider;
    uint8_t rgb;
    #define RED 1
    #define GREEN 2
    #define BLUE 4
    cycleDivider++;
    if (stateBasicAcCharging == ACOBC_STATE_INITIALIZING) {
        if (cycleDivider & 2) { rgb = GREEN; } else { rgb = 0; }
    } else if (stateBasicAcCharging == ACOBC_STATE_CHARGING) {
        rgb = BLUE;
    } else if (stateBasicAcCharging == ACOBC_STATE_CHARGEFINISHED) {
        rgb = BLUE+GREEN;
    } else if (stateBasicAcCharging == ACOBC_STATE_CHARGEERROR) {
        rgb = RED;
    } else {
        if (cycleDivider & 1) { rgb = RED; } else { rgb = 0; } /* everything else: flash red */
    }
    return rgb; 
}

void acOBC_calculateCurrentLimit(void) {
  float evseCurrentLimitAC_A;
  uint8_t cpDuty_Percent = (uint8_t)Param::GetFloat(Param::evsecp);
  /* Calculate the EVSE current limit based on the PWM ratio.
     The scaling is explained here:  https://de.wikipedia.org/wiki/IEC_62196_Typ_2 */
    if (cpDuty_Percent<8) {
        evseCurrentLimitAC_A = 0; /* below 8% is reserved for digital communication (nominal 5%). No analog current limit. */
    } else if (cpDuty_Percent<10) {
        evseCurrentLimitAC_A = 6; /* from 8% to 10% we have 6A */
    } else if (cpDuty_Percent<85) {
        evseCurrentLimitAC_A = (uint8_t)((float)cpDuty_Percent*0.6); /* from 10% to 85% the limit is 0.6A*PWM */
    } else  if (cpDuty_Percent<=96) {
        evseCurrentLimitAC_A = (uint8_t)((float)(cpDuty_Percent-64)*2.5); /* from 80% to 96% the limit is 2.5A*(PWM-64) */
    } else if (cpDuty_Percent<=97) {
        evseCurrentLimitAC_A = 80; /* 80A */
    } else {
        evseCurrentLimitAC_A = 0; /* above 97% PWM no charging allowed */
    }
    Param::SetFloat(Param::evseCurrentLimit, evseCurrentLimitAC_A);
    /* for detecting AC charging, we need to debounce. Otherwise the undefined bouncing during plug-in is detected as AC charging PWM. */
    if (evseCurrentLimitAC_A>0) {
        if (debounceCounterUntilAcValid<5) { /* 5 cycles with 100ms */
            debounceCounterUntilAcValid++;
        } else {
            acOBC_activateBasicAcCharging(); /* in case of valid AC charging PWM, we switch to the AC charging mode. */
        }
    } else {
        debounceCounterUntilAcValid=0;
    }
}

void acOBC_mainfunction(void) {
    /* cyclic main function. Runs in 100ms */
    uint8_t x;
    acOBC_calculateCurrentLimit();
    if (isBasicAcCharging) {
        /* take data from the spot value AcObcState (which was received from CAN) and transfer it to the stateBasicAcCharging if valid */
        x = Param::GetInt(Param::AcObcState);
        /* todo: do we need a plausibilization here? */
        if ((x == ACOBC_STATE_INITIALIZING) && (stateBasicAcCharging != ACOBC_STATE_INITIALIZING)) {
            /* we just arrived in INITIALIZING. Todo: Trigger the connector locking here. */
        }
        if ((x == ACOBC_STATE_CHARGING) && (stateBasicAcCharging != ACOBC_STATE_CHARGING)) {
            /* we just arrived in CHARGING. Set stateC here. */
            hardwareInterface_setStateC();
        }
        if ((x != ACOBC_STATE_CHARGING) && (stateBasicAcCharging == ACOBC_STATE_CHARGING)) {
            /* we just leaving CHARGING. Deactivate stateC here. */
            hardwareInterface_setStateB();
            /*  Todo: shall we unlock the connector here? */
        }
        /* todo: more cases for unlocking? */
        stateBasicAcCharging = x;
    } else {
        /* Todo: maybe need some cleanup actions here, if we left the AC charging. E.g. unlocking the connector? */
    }
}
