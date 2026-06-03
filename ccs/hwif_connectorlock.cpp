/* Hardware Interface for Connector Locking


 This module handles the actuation of the connector lock motor and the
 evaluation of the connector lock feedback.

 Behaviour:
  - When LockOpenThresh == LockClosedThresh: no feedback assumed, purely time-based.
    The motor runs for LockRunTime and the state is then assumed to have changed.
  - When LockOpenThresh != LockClosedThresh: feedback is used to detect when the
    closed target position is reached and stop the motor early. During opening the
    motor always runs for full LockRunTime. Feedback is interpreted as binary
    opened/closed only; no analog intermediate movement state is derived.
    In either case the motor is never run for longer than LockRunTime.
  - While moving from Open to Closed the state reports "Closing" (transition active).
  - While moving from Closed to Open the state reports "Opening" (transition active).

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
      else
         state = LOCK_UNKNOWN;
   }
   else /* (lockClosedThresh < lockOpenThresh) */
   {
      if (feedbackValue < lockClosedThresh)
         state = LOCK_CLOSED;
      else if (feedbackValue > lockOpenThresh)
         state = LOCK_OPEN;
      else
         state = LOCK_UNKNOWN;
   }

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
   bool useFeedback = (Param::GetInt(Param::LockOpenThresh) != Param::GetInt(Param::LockClosedThresh));

   /* Detect a new request and start the motor */
   if (lockRequest != lockRequestOld) {
      lockRequestOld = lockRequest;
      lockTimer = Param::GetInt(Param::LockRunTime) / 30; /* in 30ms steps */
      if (lockRequest == LOCK_OPEN) {
         Param::SetInt(Param::LockState, LOCK_OPENING);
         hardwareInteface_setHBridge(pwmNeg, pwmPos);
         addToTrace(MOD_HWIF, "unlocking the connector");
      } else if (lockRequest == LOCK_CLOSED) {
         Param::SetInt(Param::LockState, LOCK_CLOSING);
         hardwareInteface_setHBridge(pwmPos, pwmNeg);
         addToTrace(MOD_HWIF, "locking the connector");
      }
   }

   if (lockTimer > 0) {
      bool doneByFeedback = false;
      bool allowFeedbackStop = useFeedback && (lockRequest == LOCK_CLOSED);
      if (allowFeedbackStop) {
         lockState = hwIf_getLockState();
         if (lockState == lockRequest) {
            doneByFeedback = true;
         }
      }
      if (doneByFeedback) {
         lockTimer = 0;
      } else {
         lockTimer--;
      }
      if (lockTimer == 0) {
         hardwareInteface_setHBridge(0, 0);
         if (!doneByFeedback) {
            lockState = lockRequest; /* assume target reached */
            if (allowFeedbackStop) {
               addToTrace(MOD_HWIF, "connector lock timed out without feedback confirmation");
            }
         }
         Param::SetInt(Param::LockState, lockState);
         addToTrace(MOD_HWIF, "finished connector (un)locking");
      }
   }
}
