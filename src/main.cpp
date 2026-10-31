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
#include "sdocommands.h"
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
#include "hardwareVariants.h"
#include "acOBC.h"
#include "wakecontrol.h"
#include "serialRing.h"
#ifdef ENABLE_PLCBOOT
#include "plcboot.h"
#endif

#define PRINT_JSON 0

/* to solve linker warning, see https://openinverter.org/forum/viewtopic.php?p=64546#p64546 */
extern "C" void __cxa_pure_virtual()
{
    while (1);
}

static Stm32Scheduler* scheduler;
static CanHardware* can;
static CanMap* canMap;
static CanSdo* canSdo;
static volatile uint8_t catchUpArmed; /* see schedulerCatchUp() */

static void Ms100Task(void)
{
    static uint8_t runs;
    if (runs < 3)
        runs++;  /* arm the scheduler catch-up from the third run, once start-up is over */
    else
        catchUpArmed = 1;

    DigIo::led_alive.Toggle();
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
    Param::SetInt(Param::AdcIpropi, AnaIn::ipropi.Get());
    temperatures_calculateTemperatures();
    acOBC_mainfunction();
    hardwareInterface_WakeupOtherPeripherals();
    hwIf_handleInletVoltage();
    serialRing_cyclic100ms(); //reports serial lines dropped because the ring was full

    //Watchdog
    int wd = Param::GetInt(Param::CanWatchdog);
    if (wd < CAN_TIMEOUT)
    {
        Param::SetInt(Param::CanWatchdog, ++wd);
    }

    else if ((Param::GetInt(Param::DemoVoltage)>=150) && (Param::GetInt(Param::DemoVoltage)<=250) && (Param::GetInt(Param::DemoControl)==DEMOCONTROL_STANDALONE))
    {
        /* we are in demo mode without CAN. Do not set the CAN timeout error indication. */
    }
    else
    {
        /* we rely on the CAN input, so we set the error indication if the CAN messages timed out. */
        ErrorMessage::Post(ERR_CANTIMEOUT);
    }

    //CAN bus a sleep !!!to decide
    Param::SetInt(Param::CanAwake, (rtc_get_counter_val() - can->GetLastRxTimestamp()) < 200);
    wakecontrol_mainfunction();
    canMap->SendAll();
}

static void Ms30Task()
{
    spiQCA7000checkForReceivedData();
#ifdef ENABLE_PLCBOOT
    plcboot_Mainfunction();
    if (plcboot_isActive())
    {
        hardwareInterface_cyclic();
        pushbutton_handlePushbutton();
        Param::SetInt(Param::ButtonPushed, pushbutton_isPressed500ms());
        ErrorMessage::SetTime(rtc_get_counter_val());
        return;
    }
#endif
    connMgr_Mainfunction(); /* ConnectionManager */
    modemFinder_Mainfunction();
    runSlacSequencer();
    runSdpStateMachine();
    tcp_Mainfunction();
    pevStateMachine_Mainfunction();

    //cyclicLcdUpdate();
    hardwareInterface_cyclic();
    pushbutton_handlePushbutton();
    Param::SetInt(Param::ButtonPushed, pushbutton_isPressed500ms());
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
    static bool enableReceived = false;

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
        if (enableReceived)
            Param::SetInt(Param::CanWatchdog, 0);
        enableReceived = false; //this will be set back to true once enable is received again
        break;
    case Param::enable:
        //by the BMS or VCU. Whenever this AND ChargeCurrent is updated we feed the dog
        //When it is no longer updated the dog will bark and stop the charge session
        enableReceived = true;
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

/* Scheduler catch-up after a task overran.
   libopeninv's Stm32Scheduler::Run() moves a task's compare value on by one period, runs the task, and then clears the
   channel's flag. If the task runs longer than its period, the compare match for its next run happens while it runs,
   and that clear removes it: the 16-bit timer (100 kHz) reaches the compare value again only after it wraps, 655 ms
   later. Ms30Task overruns in cycles that print a lot (the serial output blocks once its buffer is full), and then
   doesn't run for about 650 ms: no modem reads, no state machine, no push button.
   This runs after Run() and moves such a task's next start to CATCHUP_GAP_TICKS from now, when all of these hold:
   - start-up is over (Ms100Task arms it from its third run; the timer is running);
   - the task ran in this interrupt (Run() moved its compare value on);
   - its match was lost: the new compare value is behind the counter by less than one period, and its flag isn't set;
   - its previous run wasn't itself a catch-up. Two overruns in a row get the old behaviour (the 655 ms wait), so at
     most one run in two is moved, never back to back, and there are always CATCHUP_GAP_TICKS free.
   It only writes the compare value: it never raises the interrupt itself (doing so can make the interrupt run forever
   while AddTask has the counter stopped) and it never prints; the main loop reports it. At worst it does nothing.
   The root cause is in libopeninv (clearing the flag after running the task); this works around it here.
   Tasks: Ms30Task on channel 1, Ms100Task on channel 2 (the order of AddTask in main). */
#define CATCHUP_GAP_TICKS 200 /* 2 ms at 100 kHz */
static const uint16_t taskPeriodTicks[2] = { 30 * 100, 100 * 100 };
static uint8_t caughtUp[2];          /* the task's next run is a catch-up */
static volatile uint16_t catchUpCount; /* catch-ups since power-on, reported by the main loop */
static volatile uint8_t catchUpTask;   /* 0 Ms30Task, 1 Ms100Task: the last one caught up */
static volatile uint16_t catchUpLateMs; /* how far its next start was already overdue */

static void schedulerCatchUp(const uint16_t* ccrBefore)
{
    if (!catchUpArmed || !(TIM_CR1(TIM4) & TIM_CR1_CEN))
        return;
    for (int i = 0; i < 2; i++)
    {
        volatile uint32_t* ccr = i == 0 ? &TIM_CCR1(TIM4) : &TIM_CCR2(TIM4);
        uint16_t now = timer_get_counter(TIM4);
        uint16_t behind = now - (uint16_t)*ccr; /* less than a period if the next start has already passed */

        if ((uint16_t)*ccr == ccrBefore[i])
            continue; /* the task didn't run in this interrupt */
        if (behind >= taskPeriodTicks[i] || timer_get_flag(TIM4, TIM_SR_CC1IF << i))
        {
            caughtUp[i] = 0; /* on time (or overdue by more than a period, or already pending): nothing to do */
            continue;
        }
        if (caughtUp[i])
        {
            caughtUp[i] = 0; /* the catch-up run overran too: leave the old behaviour */
            continue;
        }
        *ccr = (uint16_t)(now + CATCHUP_GAP_TICKS);
        caughtUp[i] = 1;
        catchUpTask = i;
        catchUpLateMs = behind / 100;
        catchUpCount++;
    }
}

/* Reports the scheduler catch-ups from the main loop, never from the interrupt. */
static void ReportCatchUps()
{
    static uint16_t reported;
    uint16_t count = catchUpCount;

    if (count != reported)
    {
        reported = count;
        printf("[%u] Scheduler: Ms%dTask overran, its next run moved forward (%d ms overdue; %u since power-on)\r\n",
               rtc_get_ms(), catchUpTask == 0 ? 30 : 100, catchUpLateMs, (unsigned)count);
    }
}

//Whichever timer(s) you use for the scheduler, you have to
//implement their ISRs here and call into the respective scheduler
extern "C" void tim4_isr(void)
{
    uint16_t ccrBefore[2] = { (uint16_t)TIM_CCR1(TIM4), (uint16_t)TIM_CCR2(TIM4) }; /* see schedulerCatchUp() */
    scheduler->Run();
    schedulerCatchUp(ccrBefore);
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
    gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_TIM3_REMAP_FULL_REMAP | AFIO_MAPR_TIM2_REMAP_FULL_REMAP);

    wakecontrol_init(); //Make sure board stays awake

    hardwareInterface_setStateB();
    hw_evaluateHardwareVariants();
    tim_setup(Param::GetInt(Param::HardwareVariant)); //Initialize CP duty cycle measurement and lock/contactor PWM
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
    RingTerminal t(UART4, termCmds); //output through a ring buffer that never blocks (ccs/serialRing.cpp)
    TerminalCommands::SetCanMap(canMap);
    SdoCommands::SetCanMap(canMap);

    printf("This is Clara version %s\r\n", VERSTR);
    printf("githubRunNumber %d\r\n", GITHUB_RUN_NUMBER);
    printf("logging is %d\r\n", Param::GetInt(Param::logging));
    SetMacAddress();

    s.AddTask(Ms30Task, 30);
    s.AddTask(Ms100Task, 100);

    //backward compatibility, version 4 was the first to support the "stream" command
    Param::SetInt(Param::version, 4);
    Param::SetInt(Param::LockState, LOCK_OPEN); //Assume lock open
    Param::SetInt(Param::VehicleSideIsoMonAllowed, 1); /* isolation monitoring on vehicle side is allowed per default */
    Param::Change(Param::PARAM_LAST); //Call callback one for general parameter propagation
    //Now all our main() does is running the terminal
    //All other processing takes place in the scheduler or other interrupt service routines
    //The terminal has lowest priority, so even loading it down heavily will not disturb
    //our more important processing routines.
    while(1)
    {
        char c = 0;
        CanSdo::SdoFrame* sdoFrame = sdo.GetPendingUserspaceSdo();

        if (0 != sdoFrame)
        {
           SdoCommands::ProcessStandardCommands(sdoFrame);
           sdo.SendSdoReply(sdoFrame);
        }

        if (sdo.GetPrintRequest() == PRINT_JSON)
        {
            TerminalCommands::PrintParamsJson(&sdo, &c);
        }
        else
        {
            PrintTrace();
            ReportCatchUps();
        }
        t.Run();
    }

    return 0;
}
