/* Hardware Interface for Connector Locking


 This module shall handle the actuation of the connector lock motor, and
 the evaluation of the connector lock feedback.

 There are different variants which we want to support.
 1. Time based locking, without evaluating the feedback
    The lock motor is actuated for a parametrizable time, no matter whether there is a feedback or not.
    Parametrization: Set LockOpenThresh and LockClosedThresh to the same value (e.g. 0), and
                     Set LockRunTime to the intended actuation time (e.g. 500ms).
 2. Locking with feedback
     We evaluate the lock feedback, by comparing the analog feedback value with the thresholds
     LockOpenThresh and LockClosedThresh.
     Different kind of feedback circuits are in the field.
     - single switch
     - dual switch
     - continuous (poti, analog hall)
     - in combination with different resistors (parallel, series)
     
     Example dual-switch with 3 resistors: https://openinverter.org/forum/viewtopic.php?p=82137#p82137
     Example single-switch without resistor: https://openinverter.org/forum/viewtopic.php?p=82141#p82141
     
     To be defined:
     - How to react if the feedback does not come? Stop motor, continue or inhibit charging, blink code, retry or not, ...

*/

#include "ccs32_globals.h"


static LockStt lockRequest;
static LockStt lockState = LOCK_OPEN; //In case we have no feedback we assume the lock is open
static uint16_t lockTimer;


void hardwareInterface_triggerConnectorLocking(void)
{
   lockRequest = LOCK_CLOSED;
}

void hardwareInterface_triggerConnectorUnlocking(void)
{
   lockRequest = LOCK_OPEN;
}

uint8_t hardwareInterface_isConnectorLocked(void)
{
   return lockState == LOCK_CLOSED; /* no matter whether we have a real lock or just a simulated one, always give the state. */
}


static LockStt hwIf_getLockState()
{
   static int lastFeedbackValue = 0;
   static uint32_t lockClosedTime = 0;
   int lockOpenThresh = Param::GetInt(Param::LockOpenThresh);
   int lockClosedThresh = Param::GetInt(Param::LockClosedThresh);
   int feedbackValue = AnaIn::lockfb.Get();
   LockStt state = LOCK_UNKNOWN;

   if (lockClosedThresh > lockOpenThresh) //Feedback value when closed is greater than when open
   {
      if (feedbackValue > lockClosedThresh)
         state = LOCK_CLOSED;
      else if (feedbackValue < lockOpenThresh)
         state = LOCK_OPEN;
      else if ((feedbackValue - lastFeedbackValue) < 10)
         state = LOCK_OPENING;
      else if ((feedbackValue - lastFeedbackValue) > 10)
         state = LOCK_CLOSING;
   }
   else if (lockClosedThresh < lockOpenThresh)
   {
      if (feedbackValue < lockClosedThresh)
         state = LOCK_CLOSED;
      else if (feedbackValue > lockOpenThresh)
         state = LOCK_OPEN;
      else if ((feedbackValue - lastFeedbackValue) > 10)
         state = LOCK_OPENING;
      else if ((feedbackValue - lastFeedbackValue) < 10)
         state = LOCK_CLOSING;
   }
   else
   {
      //When both threshold sit at the same value, disable lock feedback
      state = LOCK_UNKNOWN;
   }

   if (state == LOCK_CLOSED)
   {
      lockClosedTime = rtc_get_ms();
   }

   bool isOpening = (rtc_get_ms() - lockClosedTime) < (uint32_t)(Param::GetInt(Param::LockRunTime));
   //Report lock opening at least x ms after we left closed state
   if (state == LOCK_OPEN && isOpening)
   {
      state = LOCK_OPENING;
   }

   lastFeedbackValue = feedbackValue;

   return state;
}


void hwIf_handleLockRequests(void)
{
   int lockOpenThresh = Param::GetInt(Param::LockOpenThresh);
   int lockClosedThresh = Param::GetInt(Param::LockClosedThresh);
   /* calculation examples:
     1. LockDuty = 0%, means "no lock control". pwmNeg and pwmPos have the identical value. No effective voltage.
     2. LockDuty = 50%, means "half battery voltage". pwmNeg has 0.25*periode, pwmPos has 0.75*periode. Effective 50% voltage.
     3. LockDuty = 100%, means "full voltage". pwmNeg is 0. pwmPos = periode. Effective 100% voltage.
     Discussion reference: https://openinverter.org/forum/viewtopic.php?p=79675#p79675 */
   int pwmNeg = (CONTACT_LOCK_PERIOD / 2) - ((CONTACT_LOCK_PERIOD / 2) * Param::GetInt(Param::LockDuty)) / 100;
   int pwmPos = (CONTACT_LOCK_PERIOD / 2) + ((CONTACT_LOCK_PERIOD / 2) * Param::GetInt(Param::LockDuty)) / 100;

   //Lock drive based on actuator feedback
   if (lockClosedThresh != lockOpenThresh)
   {
      lockState = hwIf_getLockState();
      if (lockRequest == LOCK_OPEN && lockState != LOCK_OPEN)
      {
         Param::SetInt(Param::LockState, LOCK_OPENING);
         hardwareInteface_setHBridge(pwmNeg, pwmPos);
      }
      else if (lockRequest == LOCK_CLOSED && lockState != LOCK_CLOSED)
      {
         Param::SetInt(Param::LockState, LOCK_CLOSING);
         hardwareInteface_setHBridge(pwmPos, pwmNeg);
      }
      else
      {
         Param::SetInt(Param::LockState, lockState);
         hardwareInteface_setHBridge(0, 0);
      }
   }
   else //lock drive without feedback
   {
      /* connector lock just time-based, without evaluating the feedback */
      if (lockRequest == LOCK_OPEN && lockState != LOCK_OPEN)
      {
         //addToTrace(MOD_HWIF, "unlocking the connector");
         Param::SetInt(Param::LockState, LOCK_OPENING);
         hardwareInteface_setHBridge(pwmNeg, pwmPos);
         lockTimer = lockTimer == 0 ? Param::GetInt(Param::LockRunTime) / 30 : lockTimer; /* in 30ms steps */
      }
      if (lockRequest == LOCK_CLOSED && lockState != LOCK_CLOSED)
      {
         //addToTrace(MOD_HWIF, "locking the connector");
         Param::SetInt(Param::LockState, LOCK_CLOSING);
         hardwareInteface_setHBridge(pwmPos, pwmNeg);
         lockTimer = lockTimer == 0 ? Param::GetInt(Param::LockRunTime) / 30 : lockTimer; /* in 30ms steps */
      }
      if (lockTimer>0)
      {
         /* as long as the timer runs, the PWM on the lock motor is active */
         lockTimer--;
         if (lockTimer==0)
         {
            /* if the time is expired, we turn off the lock motor and report the new state */
            hardwareInteface_setHBridge(0, 0);
            lockState = lockRequest;
            Param::SetInt(Param::LockState, lockState);
            addToTrace(MOD_HWIF, "finished connector (un)locking");
         }
      }
   }
}


