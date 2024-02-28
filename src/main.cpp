/*
 * This file is part of the stm32-template project.
 *
 * Copyright (C) 2020 Johannes Huebner <dev@johanneshuebner.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/can.h>
#include <libopencm3/stm32/iwdg.h>
#include <libopencm3/stm32/desig.h>
#include "stm32_can.h"
#include "canmap.h"
#include "cansdo.h"
#include "terminal.h"
#include "params.h"
#include "hwdefs.h"
#include "digio.h"
#include "hwinit.h"
#include "anain.h"
#include "param_save.h"
#include "my_math.h"
#include "errormessage.h"
#include "printf.h"
#include "stm32scheduler.h"
#include "terminalcommands.h"
#include "my_string.h"

#include "configuration.h"
#include "connMgr.h"
#include "hardwareInterface.h"
#include "homeplug.h"
#include "ipv6.h"
#include "modemFinder.h"
#include "myHelpers.h"
#include "pevStateMachine.h"
#include "qca7000.h"
#include "tcp.h"
#include "udpChecksum.h"
#include "temperatures.h"
#include "pushbutton.h"
#include "proximitypilot.h"
#include "acOBC.h"


#define PRINT_JSON 0

/* to solve linker warning, see https://openinverter.org/forum/viewtopic.php?p=64546#p64546 */
extern "C" void __cxa_pure_virtual() { while (1); }

static Stm32Scheduler* scheduler;
static CanHardware* can;
static CanMap* canMap;
static CanSdo* canSdo;

static void Ms100Task(void)
{
   DigIo::led_out.Toggle();
   //The boot loader enables the watchdog, we have to reset it
   //at least every 2s or otherwise the controller is hard reset.
   iwdg_reset();
   //Calculate CPU load. Don't be surprised if it is zero.
   float cpuLoad = scheduler->GetCpuLoad();
   //This sets a fixed point value WITHOUT calling the parm_Change() function
   Param::SetFloat(Param::cpuload, cpuLoad / 10);
   Param::SetInt(Param::lasterr, ErrorMessage::GetLastError());
   Param::SetInt(Param::ContactorDuty, timer_get_ic_value(CONTACT_LOCK_TIMER, TIM_IC3));
   Param::SetInt(Param::AdcLockFeedback, AnaIn::lockfb.Get());
   Param::SetInt(Param::AdcHwVariant, AnaIn::hwvariant.Get());
   Param::SetInt(Param::AdcIpropi, AnaIn::ipropi.Get());
   pp_evaluateProximityPilot();
   temperatures_calculateTemperatures();
   acOBC_mainfunction();

   switch (Param::GetInt(Param::InletVtgSrc))
   {
      case IVSRC_CHARGER:
         Param::SetFixed(Param::InletVoltage, Param::Get(Param::EvseVoltage));
         break;
      case IVSRC_ANAIN:
         Param::SetFloat(Param::InletVoltage, AnaIn::udc.Get() / Param::GetFloat(Param::UdcDivider));
         break;
      default:
      case IVSRC_CAN:
         //Do nothing, value received via CAN map
         break;
   }

   //Watchdog
   int wd = Param::GetInt(Param::CanWatchdog);
   if (wd < CAN_TIMEOUT) {
      Param::SetInt(Param::CanWatchdog, ++wd);
   }
   else if ((Param::GetInt(Param::DemoVoltage)>=150) && (Param::GetInt(Param::DemoVoltage)<=250) && (Param::GetInt(Param::DemoControl)==DEMOCONTROL_STANDALONE)) {
      /* we are in demo mode without CAN. Do not set the CAN timeout error indication. */
   }
   else {
      /* we rely on the CAN input, so we set the error indication if the CAN messages timed out. */
      ErrorMessage::Post(ERR_CANTIMEOUT);
   }

   canMap->SendAll();
}

static void Ms30Task()
{
   spiQCA7000checkForReceivedData();
   connMgr_Mainfunction(); /* ConnectionManager */
   modemFinder_Mainfunction();
   runSlacSequencer();
   runSdpStateMachine();
   tcp_Mainfunction();
   pevStateMachine_Mainfunction();

   //cyclicLcdUpdate();
   hardwareInterface_cyclic();
   pushbutton_handlePushbutton();
   ErrorMessage::SetTime(rtc_get_counter_val());
}

static void SetMacAddress()
{
   uint8_t mac[6];

   mac[0] = 2; //locally administered
   mac[1] = DESIG_UNIQUE_ID0 & 0xFF;
   *((uint32_t*)&mac[2]) = DESIG_UNIQUE_ID2;

   addToTrace(MOD_HOMEPLUG, "Our MAC address: ", mac, 6);

   //more info: https://community.st.com/t5/stm32-mcus/how-to-obtain-and-use-the-stm32-96-bit-uid/ta-p/621443

   setOurMac(mac);
}

/** This function is called when the user changes a parameter */
void Param::Change(Param::PARAM_NUM paramNum)
{
   switch (paramNum)
   {
      case Param::CanSpeed:
         can->SetBaudrate((CanHardware::baudrates)Param::GetInt(Param::CanSpeed));
         break;
      case Param::NodeId:
         canSdo->SetNodeId(Param::GetInt(Param::NodeId));
         break;
      case Param::ChargeCurrent:
         //Charge current is the single most important item that must be constantly updated
         //by the BMS or VCU. Whenever it is updated we feed the dog
         //When it is no longer updated the dog will bark and stop the charge session
         Param::SetInt(Param::CanWatchdog, 0);
         break;
      default:
         //Handle general parameter changes here. Add paramNum labels for handling specific parameters
         break;
   }
}

static void PrintTrace()
{
   static int lastState = 0;
   static uint32_t lastSttPrint = 0;
   int state = Param::GetInt(Param::opmode);
   const char* label = pevSttLabels[state];

   if ((Param::GetInt(Param::logging) & MOD_PEV) && ((rtc_get_counter_val() - lastSttPrint) >= 100 || lastState != state))
   {
      lastSttPrint = rtc_get_counter_val();
      printf("[%u] In state %s. TcpRetries %u\r\n", rtc_get_ms(), label, tcp_getTotalNumberOfRetries());
      lastState = state;
   }
}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim4_isr(void)
{
   scheduler->Run();
}

extern "C" int main(void)
{
   extern const TERM_CMD termCmds[];

   clock_setup(); //Must always come first
   rtc_setup();
   ANA_IN_CONFIGURE(ANA_IN_LIST);
   DIG_IO_CONFIGURE(DIG_IO_LIST);
   AnaIn::Start(); //Starts background ADC conversion via DMA
   write_bootloader_pininit(); //Instructs boot loader to initialize certain pins
   hardwareInterface_setStateB();

   gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_TIM3_REMAP_FULL_REMAP | AFIO_MAPR_TIM2_REMAP_FULL_REMAP);

   tim_setup(); //Initialize CP duty cycle measurement and lock/contactor PWM
   nvic_setup(); //Set up some interrupts
   parm_load(); //Load stored parameters
   qca7000setup();

   Stm32Scheduler s(TIM4); //We never exit main so it's ok to put it on stack
   scheduler = &s;
   //Initialize CAN1, including interrupts. Clock must be enabled in clock_setup()
   Stm32Can c(CAN1, (CanHardware::baudrates)Param::GetInt(Param::CanSpeed));
   CanMap cm(&c);
   CanSdo sdo(&c, &cm);
   sdo.SetNodeId(Param::GetInt(Param::NodeId));
   //store a pointer for easier access
   can = &c;
   canMap = &cm;
   canSdo = &sdo;

   //This is all we need to do to set up a terminal on USART4
   Terminal t(UART4, termCmds);
   TerminalCommands::SetCanMap(canMap);

   printf("This is Clara version %s\r\n", VERSTR);
   printf("logging is %d\r\n", Param::GetInt(Param::logging));
   SetMacAddress();

   s.AddTask(Ms30Task, 30);
   s.AddTask(Ms100Task, 100);

   //backward compatibility, version 4 was the first to support the "stream" command
   Param::SetInt(Param::version, 4);
   Param::Change(Param::PARAM_LAST); //Call callback one for general parameter propagation

   //Now all our main() does is running the terminal
   //All other processing takes place in the scheduler or other interrupt service routines
   //The terminal has lowest priority, so even loading it down heavily will not disturb
   //our more important processing routines.
   while(1)
   {
      char c = 0;
      //t.Run();
      if (sdo.GetPrintRequest() == PRINT_JSON)
      {
         TerminalCommands::PrintParamsJson(&sdo, &c);
      }
      else
      {
         PrintTrace();
      }
   }

   return 0;
}

