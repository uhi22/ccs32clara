/* Hardware Interface module */

#include "ccs32_globals.h"

#define CP_DUTY_VALID_TIMER_MAX 3 /* after 3 cycles with 30ms, we consider the CP connection lost, if
                                     we do not see PWM interrupts anymore */
#define CONTACTOR_CYCLES_FOR_FULL_PWM (33/5) /* 33 cycles per second. ~200ms should be more than enough, see https://github.com/uhi22/ccs32clara/issues/22  */
#define CONTACTOR_CYCLES_SEQUENTIAL (33/3) /* ~300ms delay from one contactor to the other, to avoid high peak current consumption. https://github.com/uhi22/ccs32clara/issues/22  */

static float cpDuty_Percent;
static uint8_t cpDutyValidTimer;
static uint8_t ContactorRequest;
static int8_t ContactorOnTimer1, ContactorOnTimer2;
static uint16_t dutyContactor1, dutyContactor2;
static uint8_t LedBlinkDivider;
static LockStt lockRequest;
static LockStt lockState = LOCK_OPEN; //In case we have no feedback we assume the lock is open
static uint16_t lockTimer;
static bool actuatorTestRunning = false;

#define ACTUTEST_STATUS_IDLE 0
#define ACTUTEST_STATUS_LOCKING_TRIGGERED 1
#define ACTUTEST_STATUS_UNLOCKING_TRIGGERED 2

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
   return Param::GetInt(Param::InletVoltage);
}

int16_t hardwareInterface_getAccuVoltage(void)
{
   if ((Param::GetInt(Param::DemoVoltage)>=150) && (Param::GetInt(Param::DemoVoltage)<=250) && (Param::GetInt(Param::DemoControl)==DEMOCONTROL_STANDALONE))
   {
      /* for demonstration without an external provided target voltage, we take the value of
         the parameter DemoVoltage and put it to the target voltage variable. */
      Param::SetInt(Param::BatteryVoltage, Param::GetInt(Param::DemoVoltage));
   }
   else
   {
      /* if the demo voltage parameter is zero (which is the default) or not plausible, do not touch anything */
   }
   return Param::GetInt(Param::BatteryVoltage);
}

int16_t hardwareInterface_getChargingTargetVoltage(void)
{
   if ((Param::GetInt(Param::DemoVoltage)>=150) && (Param::GetInt(Param::DemoVoltage)<=250) && (Param::GetInt(Param::DemoControl)==DEMOCONTROL_STANDALONE))
   {
      /* for demonstration without an external provided target voltage, we take the value of
         the parameter DemoVoltage and put it to the target voltage variable. */
      Param::SetInt(Param::TargetVoltage, Param::GetInt(Param::DemoVoltage));
   }
   else
   {
      /* if the demo voltage parameter is zero (which is the default) or not plausible, do not touch anything */
   }
   return Param::GetInt(Param::TargetVoltage);
}

int16_t hardwareInterface_getChargingTargetCurrent(void)
{
   if ((Param::GetInt(Param::DemoVoltage)>=150) && (Param::GetInt(Param::DemoVoltage)<=250) && (Param::GetInt(Param::DemoControl)==DEMOCONTROL_STANDALONE))
   {
      /* for demonstration without an external provided target voltage, we use a fix value for
         the target current. During demo mode, the charger is in constant voltage mode, so the
         only important thing is that configured current can drive the load. Let's say 10A is good. */
      Param::SetInt(Param::ChargeCurrent, 10);
   }
   int16_t iOriginalDemand = Param::GetInt(Param::ChargeCurrent); /* the current demand from BMS */
   int16_t iLimit = Param::GetInt(Param::TempLimitedCurrent); /* the limit due to inlet temperature */
   int16_t iEVTarget;
   if (iOriginalDemand>iLimit) {
       /* We are limiting. Set a spot value "LimitationReason=LimitedDueToHighInletTemperature" so that the
          user has a chance to understand what happens. */
       Param::SetInt(Param::LimitationReason, LIMITATIONREASON_INLET_HOT);
       iEVTarget = iLimit;
   } else {
       int chargerLimit = Param::GetInt(Param::EvseMaxCurrent);

       //Who is the weaker link in the chain - battery or charger?
       if (iOriginalDemand >= chargerLimit)
           Param::SetInt(Param::LimitationReason, LIMITATIONREASON_CHARGER);
       else
           Param::SetInt(Param::LimitationReason, LIMITATIONREASON_BATTERY);

       /* no temperature limitation */
       iEVTarget = iOriginalDemand;
   }
   Param::SetInt(Param::EVTargetCurrent, iEVTarget);
   return iEVTarget;
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
   //Enable or disable contactor driver
   if (out1duty_4k > 0 || out2duty_4k > 0)
      DigIo::contact_out.Set();
   else
      DigIo::contact_out.Clear();

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

bool hardwareInterface_stopChargeRequested()
{
    uint8_t stopReason = STOP_REASON_NONE;
    if (pushbutton_isPressed500ms()) {
        stopReason = STOP_REASON_BUTTON;
        Param::SetInt(Param::StopReason, stopReason);
        addToTrace(MOD_HWIF, "User pressed the stop button.");
    }
    if (!Param::GetBool(Param::enable)) {
        stopReason = STOP_REASON_MISSING_ENABLE;
        Param::SetInt(Param::StopReason, stopReason);
        addToTrace(MOD_HWIF, "Got enable=false.");
    }
    if ((Param::GetInt(Param::CanWatchdog) >= CAN_TIMEOUT) && (Param::GetInt(Param::DemoControl) != DEMOCONTROL_STANDALONE)) {
        stopReason = STOP_REASON_CAN_TIMEOUT;
        Param::SetInt(Param::StopReason, stopReason);
        addToTrace(MOD_HWIF, "Timeout of CanWatchdog.");
    }
    if (Param::GetFloat(Param::TempLimitedCurrent)<0.1) { /* overheat of the inlet shall stop the session */
        stopReason = STOP_REASON_INLET_OVERHEAT;
        Param::SetInt(Param::StopReason, stopReason);
        addToTrace(MOD_HWIF, "Inlet overheated.");
    }
    return (stopReason!=STOP_REASON_NONE);
}

void hardwareInterface_resetSimulation(void)
{
   hwIf_simulatedSoc_0p01 = 2000; /* 20% */
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

static void hwIf_handleContactorRequests(void)
{
   if (ContactorRequest==0)
   {
      /* request is "OFF" -> set PWM immediately to zero for both contactors */
      dutyContactor1 = 0;
      dutyContactor2 = 0;
      ContactorOnTimer1=0;
      ContactorOnTimer2=-CONTACTOR_CYCLES_SEQUENTIAL; /* start the second contactor with a negative time, so that it will be later. */
   }
   else
   {
      /* request is "ON". Start with 100% PWM, and later switch to economizer mode */
      if (ContactorOnTimer1==0) {
         dutyContactor1 = CONTACT_LOCK_PERIOD;
         addToTrace(MOD_HWIF, "Turning on charge port contactor 1");
      }
      if (ContactorOnTimer2==0) {
         dutyContactor2 = CONTACT_LOCK_PERIOD;
         addToTrace(MOD_HWIF, "Turning on charge port contactor 2");
      }
      if (ContactorOnTimer1>=CONTACTOR_CYCLES_FOR_FULL_PWM) {
         dutyContactor1 = (Param::GetInt(Param::EconomizerDuty) * CONTACT_LOCK_PERIOD) / 100; /* reduced duty */
      }
      if (ContactorOnTimer2>=CONTACTOR_CYCLES_FOR_FULL_PWM) {
         dutyContactor2 = (Param::GetInt(Param::EconomizerDuty) * CONTACT_LOCK_PERIOD) / 100; /* reduced duty */
      }
      if (ContactorOnTimer1<127) ContactorOnTimer1++;
      if (ContactorOnTimer2<127) ContactorOnTimer2++;
   }
   hardwareInteface_setContactorPwm(dutyContactor1, dutyContactor2);
}

static void hwIf_handleLockRequests()
{
   int lockOpenThresh = Param::GetInt(Param::LockOpenThresh);
   int lockClosedThresh = Param::GetInt(Param::LockClosedThresh);
   int pwmNeg = (CONTACT_LOCK_PERIOD / 2) - (CONTACT_LOCK_PERIOD * Param::GetInt(Param::LockDuty)) / 100;
   int pwmPos = (CONTACT_LOCK_PERIOD / 2) + (CONTACT_LOCK_PERIOD * Param::GetInt(Param::LockDuty)) / 100;

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

static void handleApplicationRGBLeds(void)
{
   LedBlinkDivider++;
   if (acOBC_isBasicAcCharging()) {
       /* In case of analog AC charging, we take the LED state from the acOBC handler, and do not care for the PLC modem state. */
       hardwareInterface_setRGB(acOBC_getRGB());
       return;
   }
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
   if (checkpointNumber>=560 /* CableCheck */)
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
   if ((checkpointNumber>=570 /* PreCharge */) && (checkpointNumber<700 /* charge loop start */))
   {
      if (LedBlinkDivider & 1) /* very fast flashing during the PreCharge */
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

static void ActuatorTest()
{
   bool blTestOngoing = true;

   switch (Param::GetInt(Param::ActuatorTest))
   {
   case TEST_CLOSELOCK:
      hardwareInterface_triggerConnectorLocking();
      Param::SetInt(Param::ActuatorTest, TEST_NONE); //Make sure we only trigger the test once
      break;
   case TEST_OPENLOCK:
      hardwareInterface_triggerConnectorUnlocking();
      Param::SetInt(Param::ActuatorTest, TEST_NONE); //Make sure we only trigger the test once
      break;
   case TEST_CONTACTOR:
      hardwareInterface_setPowerRelayOn();
      break;
   case TEST_STATEC:
      hardwareInterface_setStateC();
      break;
   case TEST_LEDGREEN:
      hardwareInterface_setRGB(2);
      break;
   case TEST_LEDRED:
      hardwareInterface_setRGB(1);
      break;
   case TEST_LEDBLUE:
      hardwareInterface_setRGB(4);
      break;
   default: /* all cases including TEST_NONE are stopping the actuator test */
      blTestOngoing = false;
      if (actuatorTestRunning) {
        /* If the actuator test is just ending, then perform a clean up:
           Return everything to default state. LEDs are reset anyway as soon as we leave test mode. */
        hardwareInterface_setPowerRelayOff();
        hardwareInterface_setStateB();
        actuatorTestRunning = false;
      } else {
        /* actuator test was not ongoing and is not requested -> nothing to do */
      }
      break;
   }
   actuatorTestRunning = blTestOngoing;
}

void hardwareInterface_WakeupOtherPeripherals()
{
   static int wakeUpPulseLength = 10;
   bool dutyValid = Param::GetInt(Param::ControlPilotDuty) > 3;
   bool ppValid = Param::GetInt(Param::ResistanceProxPilot) < 2000;
   int wakeupPinFunc = Param::GetInt(Param::WakeupPinFunc);

   if (!DigIo::keep_power_on.Get()) {
      //Make sure we don't wake ourself up
      //Unless we monitor PP
      DigIo::trigger_wakeup.Clear();
      return;
   }

   switch (wakeupPinFunc) {
   case WAKEUP_LEVEL:
      DigIo::trigger_wakeup.Set(); //Wake up other peripherals
      break;
   case WAKEUP_PULSE:
      if (wakeUpPulseLength > 0) {
         wakeUpPulseLength--;
         DigIo::trigger_wakeup.Set();
      }
      else DigIo::trigger_wakeup.Clear();
      break;
   case WAKEUP_LEVEL | WAKEUP_ONVALIDCP:
      if (dutyValid) DigIo::trigger_wakeup.Set();
      else DigIo::trigger_wakeup.Clear();
      break;
   case WAKEUP_LEVEL | WAKEUP_ONVALIDPP:
      if (ppValid || dutyValid) DigIo::trigger_wakeup.Set();
      else DigIo::trigger_wakeup.Clear();
      break;
   case WAKEUP_PULSE | WAKEUP_ONVALIDCP:
      if (dutyValid && wakeUpPulseLength > 0) {
         wakeUpPulseLength--;
         DigIo::trigger_wakeup.Set();
      }
      else if (!dutyValid) {
         wakeUpPulseLength = 10; //allow pulsing again when PWM returns
         DigIo::trigger_wakeup.Clear();
      }
      else {
         DigIo::trigger_wakeup.Clear();
      }
      break;
   }
}

/* send the measured CP duty cycle and PP resistance etc to the serial console for debugging. */
void hardwareInterface_LogTheCpPpPhysicalData(void) {
      addToTrace(MOD_HWIF, "cpDuty [%] ", (int16_t)Param::GetInt(Param::ControlPilotDuty));
      addToTrace(MOD_HWIF, "AdcProximityPilot ", (int16_t)Param::GetInt(Param::AdcProximityPilot));
      addToTrace(MOD_HWIF, "ResistanceProxPilot [ohm] ", (int16_t)Param::GetInt(Param::ResistanceProxPilot));
      addToTrace(MOD_HWIF, "HardwareVariant ", (int16_t)Param::GetInt(Param::HardwareVariant));
}

void hardwareInterface_cyclic(void)
{
   uint8_t blActuatorTestAllowed;

   if (timer_get_flag(CP_TIMER, TIM_SR_CC1IF))
      cpDutyValidTimer = CP_DUTY_VALID_TIMER_MAX;

   if (cpDutyValidTimer>0)
   {
      /* we have a CP PWM capture flag seen not too long ago. Just count the timeout. */
      cpDutyValidTimer--;
      //Divide on-time by period time to arrive at duty cycle
      cpDuty_Percent = (100.0f * timer_get_ic_value(CP_TIMER, TIM_IC2)) / timer_get_ic_value(CP_TIMER, TIM_IC1);
   }
   else
   {
      /* no CP PWM capture flag. The timeout expired. We set the measured PWM and duty to zero. */
      cpDuty_Percent = 0;
   }

   Param::SetFloat(Param::ControlPilotDuty, cpDuty_Percent);

   /* Run actuator test only if we are not connected to a charger */
   blActuatorTestAllowed = (Param::GetFloat(Param::ResistanceProxPilot)>2000) && (Param::GetInt(Param::opmode) == 0);
   if (blActuatorTestAllowed) {
       /* actuator test is allowed -> run it */
       ActuatorTest();
   } else if (!blActuatorTestAllowed && actuatorTestRunning) {
       /* not allowed, but running. We cancel the actuator test. */
       Param::SetInt(Param::ActuatorTest, 0);
       ActuatorTest(); /* Run once to disable all tested outputs */
   } else {
      /* not allowed and not running -> Keep test selection reset */
      Param::SetInt(Param::ActuatorTest, 0);
   }

   //LEDs may be tested, disconnect from application
   if (!actuatorTestRunning)
      handleApplicationRGBLeds();

   hwIf_handleContactorRequests();
   hwIf_handleLockRequests();
}

void hardwareInterface_init(void)
{
   /* output initialization */
   hardwareInteface_setHBridge(0, 0); /* both low */
   hardwareInteface_setContactorPwm(0, 0); /* both off */
}


