/* Hardware Interface module */

#include "ccs32_globals.h"

#define CP_DUTY_VALID_TIMER_MAX 3 /* after 3 cycles with 30ms, we consider the CP connection lost, if
                                     we do not see PWM interrupts anymore */
#define CONTACTOR_CYCLES_FOR_FULL_PWM (33*2) /* 33 cycles per second */


static uint32_t cpDuty_Percent, cpFrequency_Hz;
static uint8_t cpDutyValidTimer;
static uint16_t hwIf_testmode;
static uint16_t hwIf_testmodeTimer;
static uint8_t hwIf_ContactorRequest;
static uint8_t hwIf_ContactorOnTimer;
static uint8_t hwIf_LedBlinkDivider;


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
   return Param::GetInt(Param::batvtg);
}

int16_t hardwareInterface_getChargingTargetVoltage(void)
{
   return Param::GetInt(Param::targetvtg);
}

int16_t hardwareInterface_getChargingTargetCurrent(void)
{
   return Param::GetInt(Param::chargecur);
}

uint8_t hardwareInterface_getSoc(void)
{
   /* SOC in percent */
   //return hwIf_simulatedSoc_0p01/100;
   return Param::GetInt(Param::soc);
}

uint8_t hardwareInterface_getIsAccuFull(void)
{
   //return (hwIf_simulatedSoc_0p01/100)>95;
   return Param::GetInt(Param::soc) > 95;
}

void hardwareInterface_setPowerRelayOn(void)
{
   hwIf_ContactorRequest=1;
}

void hardwareInterface_setPowerRelayOff(void)
{
   hwIf_ContactorRequest=0;
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

void hardwareInteface_setHBridge(uint16_t out1duty_64k, uint16_t out2duty_64k)
{
   /* Controls the H-bridge */
   /* PC6 (TIM3 ch 1) and PC7 (TIM3 ch 2) are controlling the two sides of the bridge */
   /* out1 and out2 are the PWM ratios in range 0 to 64k */

   /*Assign the new dutyCycle count to the capture compare register.*/
   timer_set_oc_value(CONTACT_LOCK_TIMER, LOCK1_CHAN, out1duty_64k);
   timer_set_oc_value(CONTACT_LOCK_TIMER, LOCK2_CHAN, out2duty_64k);
}

void hardwareInteface_setContactorPwm(uint16_t out1duty_64k, uint16_t out2duty_64k)
{
   /*Assign the new dutyCycle count to the capture compare register.*/
   timer_set_oc_value(CONTACT_LOCK_TIMER, CONTACT1_CHAN, out1duty_64k);
   timer_set_oc_value(CONTACT_LOCK_TIMER, CONTACT2_CHAN, out2duty_64k);
}

void hardwareInterface_triggerConnectorLocking(void)
{
   /* todo */
}

void hardwareInterface_triggerConnectorUnlocking(void)
{
   /* todo */
}

uint8_t hardwareInterface_isConnectorLocked(void)
{
   /* todo */
   return 1;
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

   hwIf_testmodeTimer = 10; /* rewind the timer for the next phase */
   if ((pushbutton_accumulatedButtonDigits==3411) && (hwIf_testmode==0))
   {
      hwIf_testmode=1;
   }
   switch (hwIf_testmode)
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
      hardwareInteface_setHBridge(6500, 0); /* channel 1 10% */
      break;
   case 11:
      hardwareInteface_setHBridge(32768, 0); /* channel 1 50% */
      break;
   case 12:
      hardwareInteface_setHBridge(65535, 0); /* channel 1 full */
      break;
   case 13:
      hardwareInteface_setHBridge(0, 0); /* both low */
      break;
   case 14:
      hardwareInteface_setHBridge(0, 6500); /* channel 2 10% */
      break;
   case 15:
      hardwareInteface_setHBridge(0, 32768); /* channel 2 50% */
      break;
   case 16:
      hardwareInteface_setHBridge(0, 65535); /* channel 2 full */
      break;
   case 17:
      hardwareInteface_setHBridge(0, 0); /* both low */
      break;
   case 18:
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      break;
   case 19:
      hardwareInteface_setContactorPwm(65535, 0); /* 1 on */
      break;
   case 20:
      hardwareInteface_setContactorPwm(50000, 0); /* 1 half */
      break;
   case 21:
      hardwareInteface_setContactorPwm(0, 65535); /* 2 on */
      break;
   case 22:
      hardwareInteface_setContactorPwm(0, 50000); /* 2 half */
      break;
   case 23:
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      break;
   }
   hwIf_testmode++;
   if (hwIf_testmode>23) hwIf_testmode=1;
}

static void hwIf_handleContactorRequests(void)
{
   if (hwIf_testmode!=0) return; /* in case of output test mode, decouple the application */

   if (hwIf_ContactorRequest==0)
   {
      /* request is "OFF" -> set PWM immediately to zero for both contactors */
      hardwareInteface_setContactorPwm(0, 0); /* both off */
      hwIf_ContactorOnTimer=0;
   }
   else
   {
      /* request is "ON". Start with 100% PWM, and later switch to economizer mode */
      if (hwIf_ContactorOnTimer<255) hwIf_ContactorOnTimer++;
      if (hwIf_ContactorOnTimer<CONTACTOR_CYCLES_FOR_FULL_PWM)
      {
         hardwareInteface_setContactorPwm(65535, 65535); /* both full */
         addToTrace("Turning on charge port contactors");
      }
      else
      {
         int dc = (Param::GetInt(Param::economizer) * 65535) / 100;
         hardwareInteface_setContactorPwm(dc, dc); /* both reduced */
      }
   }
}

static void handleApplicationRGBLeds(void)
{
   if (hwIf_testmode!=0) return; /* in case of output test mode, decouple the application */
   hwIf_LedBlinkDivider++;
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
      if (hwIf_LedBlinkDivider & 4)
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
      if (hwIf_LedBlinkDivider & 2)
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
      if (hwIf_LedBlinkDivider & 2)
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
      if (hwIf_LedBlinkDivider & 1)
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
   hardwareInterface_handleOutputTestMode();
}

void hardwareInterface_init(void)
{
   /* output initialization */
   hardwareInteface_setHBridge(0, 0); /* both low */
   hardwareInteface_setContactorPwm(0, 0); /* both off */
}


