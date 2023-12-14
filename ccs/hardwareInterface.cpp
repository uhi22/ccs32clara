/* Hardware Interface module */

#include "ccs32_globals.h"

#define CP_DUTY_VALID_TIMER_MAX 3 /* after 3 cycles with 30ms, we consider the CP connection lost, if
                                     we do not see PWM interrupts anymore */
#define CONTACTOR_CYCLES_FOR_FULL_PWM (33*2) /* 33 cycles per second */

static uint32_t cpDuty_Percent, cpFrequency_Hz;
static uint8_t cpDutyValidTimer;
static uint16_t testmode;
static uint16_t testmodeTimer;
static uint8_t ContactorRequest;
static uint8_t ContactorOnTimer;
static uint8_t LedBlinkDivider;
static LockStt lockRequest;
static LockStt lockState;
static LockStt lockTarget;
static uint16_t lockTimer;

void hardwareInterface_showOnDisplay(char*, char*, char*)
{

}


void hardwareInterface_initDisplay(void)
{

}

int hardwareInterface_sanityCheck()
{
   return 0; /* 0 is OK */
}

uint16_t hwIf_simulatedSoc_0p01;

void hardwareInterface_simulatePreCharge(void)
{
}

void hardwareInterface_simulateCharging(void)
{
   if (hwIf_simulatedSoc_0p01<10000)
   {
      /* simulate increasing SOC */
      hwIf_simulatedSoc_0p01++;
   }
}

int16_t hardwareInterface_getInletVoltage(void)
{
   return Param::GetInt(Param::inletvtg);
}

int16_t hardwareInterface_getAccuVoltage(void)
{
    if ((Param::GetInt(Param::demovtg)>=150) && (Param::GetInt(Param::demovtg)<=250)) {
        /* for demonstration without an external provided target voltage, we take the value of
           the parameter demovtg and put it to the target voltage variable. */
        Param::SetInt(Param::batvtg, Param::GetInt(Param::demovtg));
    } else {
        /* if the demo voltage parameter is zero (which is the default) or not plausible, do not touch anything */
    }
   return Param::GetInt(Param::batvtg);
}

int16_t hardwareInterface_getChargingTargetVoltage(void)
{
    if ((Param::GetInt(Param::demovtg)>=150) && (Param::GetInt(Param::demovtg)<=250)) {
        /* for demonstration without an external provided target voltage, we take the value of
           the parameter demovtg and put it to the target voltage variable. */
        Param::SetInt(Param::targetvtg, Param::GetInt(Param::demovtg));
    } else {
        /* if the demo voltage parameter is zero (which is the default) or not plausible, do not touch anything */
    }
    return Param::GetInt(Param::targetvtg);
}

int16_t hardwareInterface_getChargingTargetCurrent(void)
{
   return Param::GetInt(Param::chargecur);
}

uint8_t hardwareInterface_getSoc(void)
{
   /* SOC in percent */
   return Param::GetInt(Param::soc);
}

uint8_t hardwareInterface_getIsAccuFull(void)
{
   return Param::GetInt(Param::soc) > 95;
}

void hardwareInterface_setPowerRelayOn(void)
{
   ContactorRequest=1;
}

void hardwareInterface_setPowerRelayOff(void)
{
   ContactorRequest=0;
}

void hardwareInterface_setStateB(void)
{
   DigIo::statec_out.Clear();
}

void hardwareInterface_setStateC(void)
{
   DigIo::statec_out.Set();
}

void hardwareInterface_setRGB(uint8_t rgb)
{
   /* Controls the three discrete LEDs.
    * bit 0: red
    * bit 1: green
    * bit 2: blue
    */
   if (rgb & 1)
      DigIo::red_out.Set();
   else
      DigIo::red_out.Clear();

   if (rgb & 2)
      DigIo::green_out.Set();
   else
      DigIo::green_out.Clear();

   if (rgb & 4)
      DigIo::blue_out.Set();
   else
      DigIo::blue_out.Clear();
}

static void hardwareInteface_setHBridge(uint16_t out1duty_4k, uint16_t out2duty_4k)
{
   /* Controls the H-bridge */
   /* PC6 (TIM3 ch 1) and PC7 (TIM3 ch 2) are controlling the two sides of the bridge */
   /* out1 and out2 are the PWM ratios in range 0 to 64k */

   /*Assign the new dutyCycle count to the capture compare register.*/
   timer_set_oc_value(CONTACT_LOCK_TIMER, LOCK1_CHAN, out1duty_4k);
   timer_set_oc_value(CONTACT_LOCK_TIMER, LOCK2_CHAN, out2duty_4k);
}

static void hardwareInteface_setContactorPwm(uint16_t out1duty_4k, uint16_t out2duty_4k)
{
   /*Assign the new dutyCycle count to the capture compare register.*/
   timer_set_oc_value(CONTACT_LOCK_TIMER, CONTACT1_CHAN, out1duty_4k);
   timer_set_oc_value(CONTACT_LOCK_TIMER, CONTACT2_CHAN, out2duty_4k);
}

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

uint8_t hardwareInterface_getPowerRelayConfirmation(void)
{
   /* todo */
   return 1;
}

void hardwareInterface_resetSimulation(void)
{
   hwIf_simulatedSoc_0p01 = 2000; /* 20% */
}

void hardwareInterface_handleOutputTestMode(void)
{
   /* This function is used to test the outputs. */

   testmodeTimer = 10; /* rewind the timer for the next phase */
   if ((pushButton_getAccumulatedDigits() == 3411) && (testmode==0))
   {
      testmode=1;
   }
   switch (testmode)
   {
   case 0: /* no test. Do not touch the outputs and do not increment the mode. */
      return;
   case 1:
      hardwareInterface_setRGB(1); /* red */
      break;
   case 2:
      hardwareInterface_setRGB(0); /* off */
      break;
   case 3:
      hardwareInterface_setRGB(2); /* green */
      break;
   case 4:
      hardwareInterface_setRGB(0); /* off */
      break;
   case 5:
      hardwareInterface_setRGB(4); /* blue */
      break;
   case 6:
      hardwareInterface_setRGB(0); /* off */
      break;
   case 7:
      hardwareInterface_setRGB(7); /* white */
      break;
   case 8:
      hardwareInterface_setRGB(0); /* off */
      break;
   case 9:
      hardwareInteface_setHBridge(0, 0); /* both low */
      break;
   case 10:
      hardwareInteface_setHBridge(CONTACT_LOCK_PERIOD / 10, 0); /* channel 1 10% */
      break;
   case 11:
      hardwareInteface_setHBridge(CONTACT_LOCK_PERIOD / 2, 0); /* channel 1 50% */
      break;
   case 12:
      hardwareInteface_setHBridge(CONTACT_LOCK_PERIOD, 0); /* channel 1 full */
      break;
   case 13:
      hardwareInteface_setHBridge(0, 0); /* both low */
      break;
   case 14:
      hardwareInteface_setHBridge(0, CONTACT_LOCK_PERIOD / 10); /* channel 2 10% */
      break;
   case 15:
      hardwareInteface_setHBridge(0, CONTACT_LOCK_PERIOD / 2); /* channel 2 50% */
      break;
   case 16:
      hardwareInteface_setHBridge(0, CONTACT_LOCK_PERIOD); /* channel 2 full */
      break;
   case 17:
      hardwareInteface_setHBridge(0, 0); /* both low */
      break;
   case 18:
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      break;
   case 19:
      hardwareInteface_setContactorPwm(CONTACT_LOCK_PERIOD, 0); /* 1 on */
      break;
   case 20:
      hardwareInteface_setContactorPwm(CONTACT_LOCK_PERIOD / 2, 0); /* 1 half */
      break;
   case 21:
      hardwareInteface_setContactorPwm(0, CONTACT_LOCK_PERIOD); /* 2 on */
      break;
   case 22:
      hardwareInteface_setContactorPwm(0, CONTACT_LOCK_PERIOD / 2); /* 2 half */
      break;
   case 23:
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      break;
   }
   testmode++;
   if (testmode>23) testmode=1;
}

static LockStt hwIf_getLockState()
{
   static int lastFeedbackValue = 0;
   static uint32_t lockClosedTime = 0;
   int lockOpenThresh = Param::GetInt(Param::lockopenthr);
   int lockClosedThresh = Param::GetInt(Param::lockclosethr);
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

   bool isOpening = (rtc_get_ms() - lockClosedTime) < (uint32_t)(Param::GetInt(Param::lockopentm));
   //Report lock opening at least x ms after we left closed state
   if (state == LOCK_OPEN && isOpening)
   {
      state = LOCK_OPENING;
   }

   lastFeedbackValue = feedbackValue;

   return state;
}

static void hwIf_handleContactorRequests(void)
{
   if (testmode!=0) return; /* in case of output test mode, decouple the application */

   if (ContactorRequest==0)
   {
      /* request is "OFF" -> set PWM immediately to zero for both contactors */
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      ContactorOnTimer=0;
   }
   else
   {
      /* request is "ON". Start with 100% PWM, and later switch to economizer mode */
      if (ContactorOnTimer<255) ContactorOnTimer++;
      if (ContactorOnTimer<CONTACTOR_CYCLES_FOR_FULL_PWM)
      {
         hardwareInteface_setContactorPwm(CONTACT_LOCK_PERIOD, CONTACT_LOCK_PERIOD); /* both full */
         addToTrace(MOD_HWIF, "Turning on charge port contactors");
      }
      else
      {
         int dc = (Param::GetInt(Param::economizer) * CONTACT_LOCK_PERIOD) / 100;
         hardwareInteface_setContactorPwm(dc, dc); /* both reduced */
      }
   }
}

static void hwIf_handleLockRequests()
{
   if (testmode!=0) return; /* in case of output test mode, decouple the application */


   int pwmNeg = (CONTACT_LOCK_PERIOD / 2) - (CONTACT_LOCK_PERIOD * Param::GetInt(Param::lockpwm)) / 100;
   int pwmPos = (CONTACT_LOCK_PERIOD / 2) + (CONTACT_LOCK_PERIOD * Param::GetInt(Param::lockpwm)) / 100;

   #ifdef LOCKING_ACTUATION_BASED_ON_FEEDBACK
   lockState = hwIf_getLockState();
   if (lockRequest == LOCK_OPEN && lockState != LOCK_OPEN)
   {
      Param::SetInt(Param::lockstt, LOCK_OPENING);
      hardwareInteface_setHBridge(pwmNeg, pwmPos);
   }
   else if (lockRequest == LOCK_CLOSED && lockState != LOCK_CLOSED)
   {
      Param::SetInt(Param::lockstt, LOCK_CLOSING);
      hardwareInteface_setHBridge(pwmPos, pwmNeg);
   }
   else
   {
      Param::SetInt(Param::lockstt, lockState);
      hardwareInteface_setHBridge(0, 0);
   }
   #else
     pwmNeg = 0;
     pwmPos = CONTACT_LOCK_PERIOD;
     /* connector lock just time-based, without evaluating the feedback */
     if (lockRequest == LOCK_OPEN) {
         printf("[%u] unlocking the connector\r\n", rtc_get_ms());
         Param::SetInt(Param::lockstt, LOCK_OPENING);
         hardwareInteface_setHBridge(pwmNeg, pwmPos);
         lockTimer = 60; /* in 30ms steps */
         lockTarget = LOCK_OPEN;
         lockRequest = LOCK_UNKNOWN;
     }
     if (lockRequest == LOCK_CLOSED) {
         printf("[%u] locking the connector\r\n", rtc_get_ms());
         Param::SetInt(Param::lockstt, LOCK_CLOSING);
         hardwareInteface_setHBridge(pwmPos, pwmNeg);
         lockTimer = 60; /* in 30ms steps */
         lockTarget = LOCK_CLOSED;
         lockRequest = LOCK_UNKNOWN;
     }
     if (lockTimer>0) {
         /* as long as the timer runs, the PWM on the lock motor is active */
         lockTimer--;
         if (lockTimer==0) {
             /* if the time is expired, we turn off the lock motor and report the new state */
             hardwareInteface_setHBridge(0, 0);
             Param::SetInt(Param::lockstt, lockTarget);
             lockState = lockTarget;
             printf("[%u] finished connector (un)locking\r\n", rtc_get_ms());
         }
     }
   #endif
}

static void handleApplicationRGBLeds(void)
{
   if (testmode!=0) return; /* in case of output test mode, decouple the application */
   LedBlinkDivider++;
   if (checkpointNumber<100)
   {
      /* modem is sleeping (or defective), or modem search ongoing */
      hardwareInterface_setRGB(4+2+1); /* blue+green+red */
      return;
   }
   if ((checkpointNumber>=100) && (checkpointNumber<150))
   {
      /* One modem detected. This is the normal "ready" case. */
      hardwareInterface_setRGB(2); /* green */
   }
   if ((checkpointNumber>150) && (checkpointNumber<=530))
   {
      if (LedBlinkDivider & 4)
      {
         hardwareInterface_setRGB(2); /* green */
      }
      else
      {
         hardwareInterface_setRGB(0); /* off */
      }
   }
   if ((checkpointNumber>=540) /* Auth finished */ && (checkpointNumber<560 /* CableCheck */))
   {
      if (LedBlinkDivider & 2)
      {
         hardwareInterface_setRGB(2); /* green */
      }
      else
      {
         hardwareInterface_setRGB(0); /* off */
      }
   }
   if ((checkpointNumber>=560 /* CableCheck */) && (checkpointNumber<700 /* charge loop start */))
   {
      if (LedBlinkDivider & 2)
      {
         hardwareInterface_setRGB(4); /* blue */
      }
      else
      {
         hardwareInterface_setRGB(0); /* off */
      }
   }
   if ((checkpointNumber>=700 /* charge loop */) && (checkpointNumber<800 /* charge loop */))
   {
      hardwareInterface_setRGB(4); /* blue */
   }
   if ((checkpointNumber>=800 /* charge end */) && (checkpointNumber<900 /* welding detection */))
   {
      if (LedBlinkDivider & 1)
      {
         hardwareInterface_setRGB(4); /* blue */
      }
      else
      {
         hardwareInterface_setRGB(2); /* green */
      }
   }
   if (checkpointNumber==900 /* session stop */)
   {
      hardwareInterface_setRGB(4+2); /* blue+green */
   }
   if (checkpointNumber>1000)   /* error states */
   {
      hardwareInterface_setRGB(1); /* red */
   }
}

void hardwareInterface_cyclic(void)
{
   if (timer_get_flag(CP_TIMER, TIM_SR_CC1IF))
      cpDutyValidTimer = CP_DUTY_VALID_TIMER_MAX;

   if (cpDutyValidTimer>0)
   {
      /* we have a CP PWM capture flag seen not too long ago. Just count the timeout. */
      cpDutyValidTimer--;
      //Divide on-time by period time to arrive at duty cycle
      cpDuty_Percent = (100 * timer_get_ic_value(CP_TIMER, TIM_IC2)) / timer_get_ic_value(CP_TIMER, TIM_IC1);
   }
   else
   {
      /* no CP PWM capture flag. The timeout expired. We set the measured PWM and duty to zero. */
      cpDuty_Percent = 0;
      cpFrequency_Hz = 0;
   }

   Param::SetInt(Param::evsecp, cpDuty_Percent);

   handleApplicationRGBLeds();
   hwIf_handleContactorRequests();
   hwIf_handleLockRequests();
   hardwareInterface_handleOutputTestMode();
}

void hardwareInterface_init(void)
{
   /* output initialization */
   hardwareInteface_setHBridge(0, 0); /* both low */
   hardwareInteface_setContactorPwm(0, 0); /* both off */
}


