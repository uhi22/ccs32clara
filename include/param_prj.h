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

/* This file contains all parameters used in your project
 * See main.cpp on how to access them.
 * If a parameters unit is of format "0=Choice, 1=AnotherChoice" etc.
 * It will be displayed as a dropdown in the web interface
 * If it is a spot value, the decimal is translated to the name, i.e. 0 becomes "Choice"
 * If the enum values are powers of two, they will be displayed as flags, example
 * "0=None, 1=Flag1, 2=Flag2, 4=Flag3, 8=Flag4" and the value is 5.
 * It means that Flag1 and Flag3 are active -> Display "Flag1 | Flag3"
 *
 * Every parameter/value has a unique ID that must never change. This is used when loading parameters
 * from flash, so even across firmware versions saved parameters in flash can always be mapped
 * back to our list here. If a new value is added, it will receive its default value
 * because it will not be found in flash.
 * The unique ID is also used in the CAN module, to be able to recover the CAN map
 * no matter which firmware version saved it to flash.
 * Make sure to keep track of your ids and avoid duplicates. Also don't re-assign
 * IDs from deleted parameters because you will end up loading some random value
 * into your new parameter!
 * IDs are 16 bit, so 65535 is the maximum
 */

 //Define a version string of your firmware here
#define VER 0.23.B

#include "myLogging.h"

/* Entries must be ordered as follows:
   1. Saveable parameters (id != 0)
   2. Temporary parameters (id = 0)
   3. Display values
 */
//Next param id (increase when adding new parameter!): 23
//Next value Id: 2016
/*              category     name         unit       min     max     default id */
#define PARAM_LIST \
    PARAM_ENTRY(CAT_HARDWARE,udcdivider,  "dig/V",   0,      100,    10,     1   ) \
    PARAM_ENTRY(CAT_HARDWARE,economizer,  "%",       0,      100,    100,    7   ) \
    PARAM_ENTRY(CAT_HARDWARE,inletvtgsrc, IVSRC,     0,      2,      0,      8   ) \
    PARAM_ENTRY(CAT_HARDWARE,lockpwm,     "dig",    -100,    100,    30,     14  ) \
    PARAM_ENTRY(CAT_HARDWARE,lockopentm,  "ms",      0,      10000,  1000,   13  ) \
    PARAM_ENTRY(CAT_HARDWARE,lockclosethr,"dig",     0,      4095,   0,      11  ) \
    PARAM_ENTRY(CAT_HARDWARE,lockopenthr, "dig",     0,      4095,   0,      12  ) \
    PARAM_ENTRY(CAT_COMM,    nodeid,      "",        1,      63,     22,     21  ) \
    PARAM_ENTRY(CAT_COMM,    canspeed,    CANSPEEDS, 0,      4,      2,      22  ) \
    PARAM_ENTRY(CAT_CHARGE,  maxpower,    "kW",      0,      1000,   100,    17  ) \
    PARAM_ENTRY(CAT_CHARGE,  maxvtg,      "V",       0,      1000,   410,    18  ) \
    PARAM_ENTRY(CAT_CHARGE,  maxcur,      "A",       0,      500,    125,    19  ) \
    PARAM_ENTRY(CAT_CHARGE,  demovtg,     "V",       0,      500,    0,      20  ) \
    TESTP_ENTRY(CAT_CHARGE,  targetvtg,   "V",       0,      1000,   0,      3   ) \
    TESTP_ENTRY(CAT_CHARGE,  chargecur,   "A",       0,      500,    0,      4   ) \
    TESTP_ENTRY(CAT_CHARGE,  soc,         "%",       0,      100,    0,      5   ) \
    TESTP_ENTRY(CAT_CHARGE,  batvtg,      "V",       0,      1000,   0,      6   ) \
    TESTP_ENTRY(CAT_TEST,    locktest,    LOCK,      0,      2,      0,      9   ) \
    TESTP_ENTRY(CAT_TEST,    logging,     MODULES,   0,      511,    DEFAULT_LOGGINGMASK,    15  ) \
    VALUE_ENTRY(opmode,      OPMODES, 2000 ) \
    VALUE_ENTRY(version,     VERSTR,  2001 ) \
    VALUE_ENTRY(lasterr,     errorListString,  2002 ) \
    VALUE_ENTRY(evsevtg,     "V",    2006 ) \
    VALUE_ENTRY(evsecur,     "A",    2010 ) \
    VALUE_ENTRY(inletvtg,    "V",    2007 ) \
    VALUE_ENTRY(evsemaxcur,  "A",    2008 ) \
    VALUE_ENTRY(evsemaxvtg,  "V",    2009 ) \
    VALUE_ENTRY(evsecp,      "%",    2012 ) \
    VALUE_ENTRY(temp1,       "°C",   2003 ) \
    VALUE_ENTRY(temp2,       "°C",   2004 ) \
    VALUE_ENTRY(temp3,       "°C",   2005 ) \
    VALUE_ENTRY(dcsw1dc,     "%",    2013 ) \
    VALUE_ENTRY(lockfb,      "dig",  2011 ) \
    VALUE_ENTRY(lockstt,     LOCK,   2014 ) \
    VALUE_ENTRY(checkpoint,  "dig",  2015 ) \
    VALUE_ENTRY(cpuload,     "%",    2094 )


/***** Enum String definitions *****/
#define OPMODES      "0=Off, 1=Connecting, 2=Connected, 3=NegotiateProtocol, 4=SessionSetup, 5=ServiceDiscovery, \
6=PaymentSelection, 7=ContractAuthentication, 8=ParameterDiscovery, 9=ConnectorLock, 10=CableCheck, 11=Precharge, \
12=ContactorsClosed, 13=PowerDelivery, 14=CurrentDemand, 15=WeldingDetection, 16=SessionStop, 17=Finished, 18=Error"

#define IVSRC        "0=ChargerOutput, 1=AnalogInput, 2=CAN"
#define LOCK         "0=None, 1=Open, 2=Close, 3=Opening, 4=Closing"
#define MODULES      "0=None, 1=ConnMgr, 2=HwInterface, 4=Homeplug, 8=StateMachine, 16=QCA, 32=Tcp, 64=TcpTraffic, 128=IPV6, 256=ModemFinder, 511=All, 447=AllButTraffic"
#define CANSPEEDS    "0=125k, 1=250k, 2=500k, 3=800k, 4=1M"
#define CAT_HARDWARE "Hardware Config"
#define CAT_CHARGE   "Charge parameters"
#define CAT_COMM     "Communication"
#define CAT_TEST     "Testing"

#define VERSTR STRINGIFY(4=VER)

/***** enums ******/

enum _canperiods
{
   CAN_PERIOD_100MS = 0,
   CAN_PERIOD_10MS,
   CAN_PERIOD_LAST
};

enum _modes
{
   MOD_OFF = 0,
   MOD_RUN,
   MOD_LAST
};

enum _inletsources
{
   IVSRC_CHARGER,
   IVSRC_ANAIN,
   IVSRC_CAN
};

//Generated enum-string for possible errors
extern const char* errorListString;

