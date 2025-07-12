/* Hardware Interface for Connector Locking


 This module shall handle the actuation of the connector lock motor, and
 the evaluation of the connector lock feedback.

 There are different variants which we want to support.
 1. Time based locking, without evaluating the feedback
    The lock motor is actuated for a parametrizable time, no matter whether there is a feedback or not.
    Parametrization: (old)Set LockOpenThresh and LockClosedThresh to the same value (e.g. 0)
                     (new)Set LockControlVariant = TimeBased_VirtualFeedback
                     and
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
static LockStt lockRequestOld=LOCK_UNKNOWN; /* old value is invalid at the beginning */
static LockStt lockState = LOCK_OPEN; //In case we have no feedback we assume the lock is open
static LockStt lastApplicationLockRequest;
static uint16_t lockTimer;
static uint8_t oldkindOfIoControl = IOCONTROL_LOCK_RETURN_CONTROL_TO_ECU;

/* The lock control via actuator test. */
void hwIf_connectorLockActuatorTest(uint8_t kindOfControl) {
  if (kindOfControl != oldkindOfIoControl) {
      switch (kindOfControl) {
          case IOCONTROL_LOCK_CLOSE:
            /* The actuator test shall control the lock motor, no matter what the state is.
               So we say: it is unlocked and shall be locked. */
            lockState = LOCK_OPEN;
            lockRequest = LOCK_CLOSED;
            break;
          case IOCONTROL_LOCK_OPEN:
            /* The actuator test shall control the lock motor, no matter what the state is.
               So we say: it is locked and shall be unlocked. */
            lockState = LOCK_CLOSED;
            lockRequest = LOCK_OPEN;
            break;
          case IOCONTROL_LOCK_RETURN_CONTROL_TO_ECU:
            /* End of the actuator test. Now the potentially stored request from the
               application can be executed. */
            lockRequest = lastApplicationLockRequest;
            break;
      }
      oldkindOfIoControl = kindOfControl;
  }
}

/* The locking via application. */
void hardwareInterface_triggerConnectorLocking(void)
{
  if (oldkindOfIoControl == IOCONTROL_LOCK_RETURN_CONTROL_TO_ECU) {
    /* only accept the control by the application, if no actuator test is ongoing. */
    lockRequest = LOCK_CLOSED;
  }
  /* even if the actuator test is blocking the application request, store it for later execution. */
  lastApplicationLockRequest = LOCK_CLOSED;
}

/* The unlocking via application. */
void hardwareInterface_triggerConnectorUnlocking(void)
{
  if (oldkindOfIoControl == IOCONTROL_LOCK_RETURN_CONTROL_TO_ECU) {
    /* only accept the control by the application, if no actuator test is ongoing. */
    lockRequest = LOCK_OPEN;
  }
  /* even if the actuator test is blocking the application request, store it for later execution. */
  lastApplicationLockRequest = LOCK_OPEN;
}

uint8_t hardwareInterface_isConnectorLocked(void)
{
   return lockState == LOCK_CLOSED; /* no matter whether we have a real lock or just a simulated one, always give the state. */
}


static LockStt hwIf_getLockState()
{
   static int lastFeedbackValue = 0;
   int lockOpenThresh = Param::GetInt(Param::LockOpenThresh);
   int lockClosedThresh = Param::GetInt(Param::LockClosedThresh);
   int feedbackValue = AnaIn::lockfb.Get();
   LockStt state = LOCK_UNKNOWN;

   if (lockClosedThresh >= lockOpenThresh) //Feedback value when closed is greater than when open
   {
      if (feedbackValue > lockClosedThresh)
         state = LOCK_CLOSED;
      else if (feedbackValue < lockOpenThresh)
         state = LOCK_OPEN;
      else if ((feedbackValue - lastFeedbackValue) < 10) /* purpose not clear. A static feedback "in the middle" would lead to "opening". */
         state = LOCK_OPENING;
      else if ((feedbackValue - lastFeedbackValue) > 10)
         state = LOCK_CLOSING;
   }
   else /* (lockClosedThresh < lockOpenThresh) */
   {
      if (feedbackValue < lockClosedThresh)
         state = LOCK_CLOSED;
      else if (feedbackValue > lockOpenThresh)
         state = LOCK_OPEN;
      else if ((feedbackValue - lastFeedbackValue) > 10)
         state = LOCK_OPENING;
      else if ((feedbackValue - lastFeedbackValue) < 10) /* purpose not clear. A static feedback "in the middle" would lead to "closing". */
         state = LOCK_CLOSING;
   }

#ifdef UNCLEAR_WHAT_WAS_THE_INTENTION_OF_THIS
   static uint32_t lockClosedTime = 0;
    /* unclear intention.
       Looks like a requirement "When changing from CLOSED to OPEN, there shall be a OPENING in between."
       But why? Is it needed for a certain consumer of the state on CAN? Or for the internal logic of Clara?
     */
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
#endif

   lastFeedbackValue = feedbackValue;

   return state;
}

/* cyclic mainfunction for the lock handling. Called in 30ms task. */
void hwIf_handleLockRequests(void)
{
  /* calculation examples:
     1. LockDuty = 0%, means "no lock control". pwmNeg and pwmPos have the identical value. No effective voltage.
     2. LockDuty = 50%, means "half battery voltage". pwmNeg has 0.25*periode, pwmPos has 0.75*periode. Effective 50% voltage.
     3. LockDuty = 100%, means "full voltage". pwmNeg is 0. pwmPos = periode. Effective 100% voltage.
     Discussion reference: https://openinverter.org/forum/viewtopic.php?p=79675#p79675 */
  int pwmNeg = (CONTACT_LOCK_PERIOD / 2) - ((CONTACT_LOCK_PERIOD / 2) * Param::GetInt(Param::LockDuty)) / 100;
  int pwmPos = (CONTACT_LOCK_PERIOD / 2) + ((CONTACT_LOCK_PERIOD / 2) * Param::GetInt(Param::LockDuty)) / 100;

  switch(Param::GetInt(Param::LockControlVariant)) {
      case LOCKCONTROL_V_FEEDBACKBASED:
        /* Lock drive based on actuator feedback */
        /* This is the old, dangerous variant. It controls the lock as long as the feedback does not
           say that the intended position is reached. Risk for burning the actuator. Discussed here:
           https://openinverter.org/forum/viewtopic.php?p=82103#p82103 and
           https://github.com/uhi22/ccs32clara/issues/51 */
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
        break;
      case LOCKCONTROL_V_TIMEBASED_VIRTUALFEEDBACK:
        /* Lock drive is pure time-based. The hardware feedback is completely igored. */
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
        break;
      case LOCKCONTROL_V_TIMEBASED_HWFEEDBACK:
        /* Lock drive is pure time-based. The hardware feedback is permanently routed to the spotvalue LockState. */
        if ((lockRequest == LOCK_OPEN) && (lockRequest != lockRequestOld)) {
            addToTrace(MOD_HWIF, "unlocking the connector (time-based)");
            hardwareInteface_setHBridge(pwmNeg, pwmPos);
            lockTimer = Param::GetInt(Param::LockRunTime) / 30; /* in 30ms steps */
        }
        if ((lockRequest == LOCK_CLOSED) && (lockRequest != lockRequestOld)) {
            addToTrace(MOD_HWIF, "locking the connector (time-based)");
            hardwareInteface_setHBridge(pwmPos, pwmNeg);
            lockTimer = Param::GetInt(Param::LockRunTime) / 30; /* in 30ms steps */
        }
        lockRequestOld = lockRequest; /* request has been translated, mark as "done" for the next loop */
        if (lockTimer>0) {
            /* as long as the timer runs, the PWM on the lock motor is active */
            lockTimer--;
            if (lockTimer==0) {
                /* if the time is expired, we turn off the lock motor */
                hardwareInteface_setHBridge(0, 0);
                addToTrace(MOD_HWIF, "finished connector (un)locking");
            }
        }
        /* permanently route the hardware feedback to the spot value */
        lockState = hwIf_getLockState();
        Param::SetInt(Param::LockState, lockState);
        break;
   }
}


