
/* AC Onboard charger handling  */

/* This module handles the communication to an onboard AC charger.
   Reception and decoding of the status message of the OBC, to use it for StateC, LEDs and connector locking.
*/

#include "ccs32_globals.h"
#include <math.h> /* for log() */

#define MAX_ADC_VALUE 4095 /* we have 12 bit ADC resolution */


static uint8_t isBasicAcCharging = 0; /* shows whether we are connected to a basic AC charger (CP PWM 8% to 96%) */
static uint8_t previousStateBasicAcCharging = OBC_IDLE;

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


static uint8_t debounceCounterUntilAcValid = 0;
static uint8_t debounceCounterFivePercentPwm = 0;
static bool ButtonStopAC = false;
static uint16_t StopAcTimer = 0;

uint8_t acOBC_isBasicAcCharging(void)
{
    return isBasicAcCharging;
}

static void evaluateProximityPilot(void)
{
    float temp = AnaIn::pp.Get();
    float U_refAdc, U_pull, U_meas, Rv, R;
    float iLimit;
    uint8_t blPlugPresent;

    if (wakecontrol_isPpMeasurementInvalid()) {
        /* in case there are conditions which corrupt the PP resistance measurement, we discard the measurement,
        and just live with the last valid value. Strategy: Better an old, valid value than a corrupted value. */
        return;
    }

    /* Step 1: Provide the raw AD value (0 to 4095) for analysis purposes. */
    Param::SetFloat(Param::AdcProximityPilot, temp);

    /* Step 2: Calculate the PP resistance */
    /* Todo: consider the input circuit variant:
         - old Foccci v4.1 has x ohm pull-up to 3V3 and no pull-down
         - next Foccci v4.2 has 330 ohm pull-up to 5V and a 3k pull-down and a 47k by 47k divider to match the 3V3 ADC input.
         - there could be an additional pull-down in the CCS inlet.
       The parameter ppvariant shall select which hardware circuit is installed. */

    if (Param::GetInt(Param::ppvariant) == 0)
    {
        /* The old Foccci (until revision 4.1) */
        U_refAdc = 3.3; /* volt. The full-scale voltage of the ADC */
        U_pull = 3.3; /* volt. The effective pull-up-voltage, created by pull-up and pull-down resistor. In this case, no pull-down at all. */
        U_meas = U_refAdc * temp/4095; /* The measured voltage on the PP. In this case no divider. */
        Rv = 1000.0; /* The effective pull-up resistor. In this case 1k. */
        /* Todo: verify on hardware */
    }
    else if (Param::GetInt(Param::ppvariant) == 1)
    {
        /* The newer Foccci with 330 ohm pull-up to 5V and a 3k pull-down and a 47k by 47k divider */
        U_refAdc = 3.3; /* volt. The full-scale voltage of the ADC */
        U_pull = 4.5; /* volt. The effective pull-up-voltage, created by pull-up and pull-down resistor. */
        U_meas = U_refAdc * temp/4095 * (47.0+47.0)/47.0; /* The measured voltage on the PP. In this case the ADC gets half the PP voltage. */
        Rv = 1 / (1/330.0 + 1/3000.0); /* The effective pull-up resistor. In this case 330ohm parallel 3000ohm. */
        /* tested on hand-wired Foccci. Calculation is ok. */
    }
    else
    {
        /* The newer Foccci with 330 ohm pull-up to 5V and no pull-down and a 47k by 47k divider */
        U_refAdc = 3.3; /* volt. The full-scale voltage of the ADC */
        U_pull = 5.0; /* volt. The effective pull-up-voltage, created by pull-up and pull-down resistor. In this case, no pull-down at all. */
        U_meas = U_refAdc * temp/4095 * (47.0+47.0)/47.0; /* The measured voltage on the PP. In this case the ADC gets half the PP voltage. */
        Rv = 330.0; /* The effective pull-up resistor. In this case 330ohm. */
        /* Todo: verify on hardware */
    }
    if (U_meas>(U_pull-0.5))
    {
        /* The voltage on PP is quite high. We do not calculate the R, because this may lead to overflow. Just
           give a high resistance value. */
        R = 10000.0; /* ohms */
    }
    else if (U_meas<0.05)
    {
        /* The voltage on PP is very low. We do not calculate the R, because this may lead to division-by-zero. Just say R=0. */
        R = 0.0; /* ohms */
    }
    else
    {
        /* calculate the resistance of the PP */
        R = Rv / (U_pull/U_meas-1);
    }
    Param::SetFloat(Param::ResistanceProxPilot, R);

    /* Step 3: Calculate the cable current limit */
    /* Reference: https://www.goingelectric.de/wiki/Typ2-Signalisierung-und-Steckercodierung/ */
    iLimit = 0;
    if ((R>1500*0.8) && (R<1500*1.2))
    {
        iLimit = 13; /* 1k5 is 13A, or digital communication */
    }
    if ((R>680*0.8) &&  (R<680*1.2))
    {
        iLimit = 20; /* 680ohm is 20A */
    }
    if ((R>220*0.8) && (R<220*1.2))
    {
        iLimit = 32; /* 220ohm is 32A */
    }
    if ((R>100*0.8) && (R<100*1.2))
    {
        iLimit = 63; /* 100ohm is 63A */
    }
    Param::SetFloat(Param::CableCurrentLimit, iLimit);

    /* Step 4: Provide the PlugPresent (e.g. for the feature "driveInhibit") */
    blPlugPresent = R < 5000; /* Design decision: The plug is considered as present if the PP resistance is below 5kohms. */
    Param::SetInt(Param::PlugPresent, blPlugPresent); /* 0: no plug present. 1: plug detected. */

}

uint8_t acOBC_getRGB(void)
{
    static uint8_t cycleDivider;
    uint8_t rgb;
#define RED 1
#define GREEN 2
#define BLUE 4
    cycleDivider++;

    if (previousStateBasicAcCharging == OBC_IDLE)
    {
        if (cycleDivider & 2)
        {
            rgb = BLUE;
        }
        else
        {
            rgb = 0;
        }
    }
    else if (previousStateBasicAcCharging == OBC_LOCK)
    {

        rgb = BLUE;
    }
    else if (previousStateBasicAcCharging == OBC_CHARGE)
    {
        if (cycleDivider & 2)
        {
            rgb = GREEN;
        }
        else
        {
            rgb = 0;
        }
    }
    else if (previousStateBasicAcCharging == OBC_COMPLETE)
    {
        rgb = BLUE+GREEN;
    }
    else if (previousStateBasicAcCharging == OBC_ERROR)
    {
        rgb = RED;
    }
    else
    {
        if (cycleDivider & 1)
        {
            rgb = RED;    /* everything else: flash red */
        }
        else
        {
            rgb = 0;
        }
    }
    return rgb;
}

void acOBC_calculateCurrentLimit(void)
{
    float evseCurrentLimitAC_A;
    uint8_t cpDuty_Percent = (uint8_t)Param::GetFloat(Param::ControlPilotDuty);
    /* Calculate the EVSE current limit based on the PWM ratio.
       The scaling is explained here:  https://de.wikipedia.org/wiki/IEC_62196_Typ_2 */
    if (cpDuty_Percent<8)
    {
        evseCurrentLimitAC_A = 0; /* below 8% is reserved for digital communication (nominal 5%). No analog current limit. */
        if (debounceCounterFivePercentPwm<5)   /* 5 cycles with 100ms */
        {
            debounceCounterFivePercentPwm++;
        }
        else
        {
            isBasicAcCharging = false;  /* in case we have stable 5% PWM, this is no basic AC charging. */

            // If the charger or VCU does not communicate with Foccci go straight to complete mode
            // as soon as an invalid/zero AC current limit is detected
            if (Param::GetInt(Param::AcChargeControl) == ACM_STANDALONE)
               Param::SetInt(Param::AcObcState, OBC_COMPLETE);
        }
    }
    else if (cpDuty_Percent<10)
    {
        evseCurrentLimitAC_A = 6; /* from 8% to 10% we have 6A */
        debounceCounterFivePercentPwm = 0;
    }
    else if (cpDuty_Percent<85)
    {
        evseCurrentLimitAC_A = cpDuty_Percent * 0.6f; /* from 10% to 85% the limit is 0.6A*PWM */
        debounceCounterFivePercentPwm = 0;
    }
    else  if (cpDuty_Percent<=96)
    {
        evseCurrentLimitAC_A = (cpDuty_Percent - 64) * 2.5f; /* from 80% to 96% the limit is 2.5A*(PWM-64) */
        debounceCounterFivePercentPwm = 0;
    }
    else if (cpDuty_Percent<=97)
    {
        evseCurrentLimitAC_A = 80; /* 80A */
        debounceCounterFivePercentPwm = 0;
    }
    else
    {
        evseCurrentLimitAC_A = 0; /* above 97% PWM no charging allowed */
        debounceCounterFivePercentPwm=0;
    }
    Param::SetFloat(Param::EvseAcCurrentLimit, evseCurrentLimitAC_A);
    /* for detecting AC charging, we need to debounce. Otherwise the undefined bouncing during plug-in is detected as AC charging PWM. */
    if (evseCurrentLimitAC_A>0)
    {
        if (debounceCounterUntilAcValid<5)   /* 5 cycles with 100ms */
        {
            debounceCounterUntilAcValid++;
        }
        else
        {
            isBasicAcCharging = true; /* in case of valid AC charging PWM, we switch to the AC charging mode. */

            // If the charger or VCU does not communicate with Foccci go straight to charge mode
            // as soon as valid AC current limit is detected
            if (Param::GetInt(Param::AcChargeControl) == ACM_STANDALONE)
               Param::SetInt(Param::AcObcState, OBC_CHARGE);
        }
    }
    else
    {
        debounceCounterUntilAcValid=0;
    }
}

static void triggerActions()
{
    Param::SetInt(Param::BasicAcCharging, isBasicAcCharging);

    //This means we are seeing CP PWM > 8%
    if (isBasicAcCharging)
    {
        uint8_t requestedState = Param::GetInt(Param::AcObcState);

        if (pushbutton_isPressed500ms())//if push button is pressed we stop charging
        {
            if(requestedState != OBC_IDLE)
            {
                requestedState = OBC_IDLE;
                ButtonStopAC = true; //we have stopped due to button press
                StopAcTimer = 0; //reset wait timer
            }
        }

        if (ButtonStopAC)
        {
            requestedState = OBC_IDLE; // force idle when we are stopped due to button
            StopAcTimer++; //increase very 100ms
            if(StopAcTimer > 150)//10s time out before starting charge again
            {
                ButtonStopAC = false; //Release the charging block
            }
        }

        if (!Param::GetBool(Param::enable)) //If the param enable is not set do not allow charging of any kind
        {
            requestedState = OBC_IDLE;
        }

        switch (requestedState)
        {
        case OBC_IDLE:
            //always turn off stateB here
            hardwareInterface_setStateB();
            Param::SetInt(Param::PortState, PS_READY); //set PortState to Ready, combined with CP current limit signaling to VCU for AC charging
            //Unlock connector when coming here from any other state
            if (Param::GetInt(Param::LockState) == LOCK_CLOSED && Param::GetBool(Param::AllowUnlock))//only unlock if allowed and lock is locked
            {
                hardwareInterface_triggerConnectorUnlocking();
            }
            break;
        case OBC_LOCK:
            if (Param::GetInt(Param::LockState) == LOCK_OPEN)//only trigger is lock is open
            {
                hardwareInterface_triggerConnectorLocking();
            }
            break;
        case OBC_PAUSE:
            break;
        case OBC_ERROR:
            Param::SetInt(Param::PortState, PS_PORTERROR); //set PortState to Error
            break;
        case OBC_COMPLETE:
            //always turn off stateC in these states
            hardwareInterface_setStateB();
            break;
        case OBC_CHARGE:
            if (Param::GetInt(Param::LockState) == LOCK_OPEN)//only trigger is lock is open
            {
                hardwareInterface_triggerConnectorLocking();
            }
            if(Param::GetInt(Param::LockState) == LOCK_CLOSED) //Do not allow Request for AC power without locking
            {
                hardwareInterface_setStateC();
                Param::SetInt(Param::PortState, PS_CHARGING_AC); //set PortState to AcCharging
            }
            break;
        }

        previousStateBasicAcCharging = requestedState;
    }
    else if (Param::GetBool(Param::PlugPresent) && Param::GetInt(Param::opmode) == 0) //Plugged in and no CCS going
    {
        Param::SetInt(Param::PortState, PS_PLUGGEDIN); //set PortState to Plugged In due to dropping way of valid CP - !!!Should not interfere with CCS charging
        int cpDuty_Percent = Param::GetInt(Param::ControlPilotDuty);
        if(cpDuty_Percent<3)
        {
            if (Param::GetInt(Param::LockState) == LOCK_CLOSED && Param::GetBool(Param::AllowUnlock))//only unlock if allowed and lock is locked
            {
                hardwareInterface_triggerConnectorUnlocking();
            }
        }
    }
    //Not Plugged in and no CCS going (just to make sure we don't interfere with CCS if PP is not connected)
    else if (!Param::GetBool(Param::PlugPresent) && Param::GetInt(Param::opmode) == 0)
    {
        hardwareInterface_setStateB();
        Param::SetInt(Param::PortState, PS_IDLE);
        if (Param::GetInt(Param::LockState) == LOCK_CLOSED) //always unlock when nothing is plugged in
        {
            hardwareInterface_triggerConnectorUnlocking();
        }
    }
}

void acOBC_mainfunction(void)//Called every 100ms
{
    /* cyclic main function. Runs in 100ms */
    acOBC_calculateCurrentLimit();
    evaluateProximityPilot();
    triggerActions();
}
