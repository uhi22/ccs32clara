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
static bool actuatorTestRunning = false;


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

void hardwareInterface_setPowerRelaysOn(uint8_t contactorSelection)
{
   ContactorRequest=contactorSelection; /* 1 or 2 for single contactor, or 3 for both contactors */
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

void hardwareInteface_setHBridge(uint16_t out1duty_4k, uint16_t out2duty_4k)
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


static void hwIf_handleContactorRequests(void)
{
  if (ContactorRequest & 1) {
      /* contactor 1 is requested */
      /* request is "ON". Start with 100% PWM, and later switch to economizer mode */
      if (ContactorOnTimer1==0) {
         dutyContactor1 = CONTACT_LOCK_PERIOD;
         addToTrace(MOD_HWIF, "Turning on charge port contactor 1");
      }
      if (ContactorOnTimer1>=CONTACTOR_CYCLES_FOR_FULL_PWM) {
         dutyContactor1 = (Param::GetInt(Param::EconomizerDuty) * CONTACT_LOCK_PERIOD) / 100; /* reduced duty */
      }
      if (ContactorOnTimer1<127) ContactorOnTimer1++;
  } else {
      /* request is "OFF" -> set PWM immediately to zero */
      dutyContactor1 = 0;
      ContactorOnTimer1=0;
  }
  
  if (ContactorRequest & 2) {
      /* contactor 2 is requested */
      if (ContactorOnTimer2==0) {
         dutyContactor2 = CONTACT_LOCK_PERIOD;
         addToTrace(MOD_HWIF, "Turning on charge port contactor 2");
      }
      if (ContactorOnTimer2>=CONTACTOR_CYCLES_FOR_FULL_PWM) {
         dutyContactor2 = (Param::GetInt(Param::EconomizerDuty) * CONTACT_LOCK_PERIOD) / 100; /* reduced duty */
      }
      if (ContactorOnTimer2<127) ContactorOnTimer2++;
  } else {
      /* request is "OFF" -> set PWM immediately to zero */
      dutyContactor2 = 0;
      ContactorOnTimer2=-CONTACTOR_CYCLES_SEQUENTIAL; /* start the second contactor with a negative time, so that it will be later. */
  }
  hardwareInteface_setContactorPwm(dutyContactor1, dutyContactor2);
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
      hardwareInterface_setRGB(RGB_WHITE);
      return;
   }
   else if ((checkpointNumber>=100) && (checkpointNumber<150))
   {
      /* One modem detected. This is the normal "ready" case. */
      hardwareInterface_setRGB(RGB_GREEN);
   }
   else if ((checkpointNumber>150) && (checkpointNumber<=530))
   {
      if (LedBlinkDivider & 4)
      {
         hardwareInterface_setRGB(RGB_GREEN);
      }
      else
      {
         hardwareInterface_setRGB(RGB_OFF);
      }
   }
   else if ((checkpointNumber>=540) /* Auth finished */ && (checkpointNumber<560 /* CableCheck */))
   {
      if (LedBlinkDivider & 2)
      {
         hardwareInterface_setRGB(RGB_GREEN);
      }
      else
      {
         hardwareInterface_setRGB(RGB_OFF);
      }
   }
   else if (checkpointNumber>=560 /* CableCheck */)
   {
      if (LedBlinkDivider & 2)
      {
         hardwareInterface_setRGB(RGB_BLUE);
      }
      else
      {
         hardwareInterface_setRGB(RGB_OFF);
      }
   }
   else if ((checkpointNumber>=570 /* PreCharge */) && (checkpointNumber<700 /* charge loop start */))
   {
      if (LedBlinkDivider & 1) /* very fast flashing during the PreCharge */
      {
         hardwareInterface_setRGB(RGB_BLUE);
      }
      else
      {
         hardwareInterface_setRGB(RGB_OFF);
      }
   }
   else if ((checkpointNumber>=700 /* charge loop */) && (checkpointNumber<800 /* charge loop */))
   {
      hardwareInterface_setRGB(RGB_BLUE);
   }
   else if ((checkpointNumber>=800 /* charge end */) && (checkpointNumber<900 /* welding detection */))
   {
      if (LedBlinkDivider & 1)
      {
         hardwareInterface_setRGB(RGB_BLUE);
      }
      else
      {
         hardwareInterface_setRGB(RGB_GREEN);
      }
   }
   else if (checkpointNumber==900 /* session stop */)
   {
      hardwareInterface_setRGB(RGB_CYAN);
   }
   else if (checkpointNumber>1000)   /* error states */
   {
      hardwareInterface_setRGB(RGB_RED);
   }
}

static void ActuatorTest()
{
   bool blTestOngoing = true;

   switch (Param::GetInt(Param::ActuatorTest))
   {
   case TEST_CLOSELOCK:
      hwIf_connectorLockActuatorTest(IOCONTROL_LOCK_CLOSE);
      break;
   case TEST_OPENLOCK:
      hwIf_connectorLockActuatorTest(IOCONTROL_LOCK_OPEN);
      break;
   case TEST_CONTACTOR1:
      hardwareInterface_setPowerRelaysOn(1);
      break;
   case TEST_CONTACTOR2:
      hardwareInterface_setPowerRelaysOn(2);
      break;
   case TEST_CONTACTORBOTH:
      hardwareInterface_setPowerRelaysOn(BOTH_CONTACTORS);
      break;
   case TEST_STATEC:
      hardwareInterface_setStateC();
      break;
   case TEST_LEDGREEN:
      hardwareInterface_setRGB(RGB_GREEN);
      break;
   case TEST_LEDRED:
      hardwareInterface_setRGB(RGB_RED);
      break;
   case TEST_LEDBLUE:
      hardwareInterface_setRGB(RGB_BLUE);
      break;
   default: /* all cases including TEST_NONE are stopping the actuator test */
      blTestOngoing = false;
      if (actuatorTestRunning) {
        /* If the actuator test is just ending, then perform a clean up:
           Return everything to default state. LEDs are reset anyway as soon as we leave test mode. */
        hardwareInterface_setPowerRelayOff();
        hardwareInterface_setStateB();
        hwIf_connectorLockActuatorTest(IOCONTROL_LOCK_RETURN_CONTROL_TO_ECU);
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
   static uint8_t wakeUpPulseLength = 10;
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
   /* make an exception: the lock/unlock actuator test shall always be possible, to be able to test also
      while the plug is inserted. */
   if ((Param::GetInt(Param::ActuatorTest)==TEST_CLOSELOCK) ||
       (Param::GetInt(Param::ActuatorTest)==TEST_OPENLOCK)) { 
       blActuatorTestAllowed = 1;
   }
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


