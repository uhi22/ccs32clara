/* Connection Manager */

/* This module is informed by the several state machines in case of good connection.
   It calculates an overall ConnectionLevel.
   This ConnectionLevel is provided to the state machines, so that each state machine
   has the possiblity to decide whether it needs to do something or just stays silent.

   The basic rule is, that a good connection on higher layer (e.g. TCP) implicitely
   confirms the good connection on lower layer (e.g. Modem presence). This means,
   the lower-layer state machine can stay silent as long as the upper layers are working
   fine.
*/

#include "ccs32_globals.h"

uint16_t connMgr_timerEthLink;
uint16_t connMgr_timerModemLocal;
uint16_t connMgr_timerModemRemote;
uint16_t connMgr_timerSlac;
uint16_t connMgr_timerSDP;
uint16_t connMgr_timerTCP;
uint16_t connMgr_timerAppl;
uint16_t connMgr_ConnectionLevel;
uint16_t connMgr_ConnectionLevelOld;
uint16_t connMgr_cycles;

#define CONNMGR_CYCLES_PER_SECOND 33 /* 33 cycles per second, because 30ms cycle time */
#define CONNMGR_TIMER_MAX (5*33) /* 5 seconds until an OkReport is forgotten. */
#define CONNMGR_TIMER_MAX_10s (10*33) /* 10 seconds until an OkReport is forgotten. */
#define CONNMGR_TIMER_MAX_15s (15*33) /* 15 seconds until an OkReport is forgotten. */
#define CONNMGR_TIMER_MAX_20s (20*33) /* 20 seconds until an OkReport is forgotten. */


uint8_t connMgr_getConnectionLevel(void)
{
   return connMgr_ConnectionLevel;
}

void connMgr_printDebugInfos(void)
{
   if (Param::GetInt(Param::logging) & MOD_CONNMGR)
      printf("[%u] [CONNMGR] %d %d %d %d %d %d %d --> %d\r\n",
             rtc_get_ms(),
             connMgr_timerEthLink,
             connMgr_timerModemLocal,
             connMgr_timerModemRemote,
             connMgr_timerSlac,
             connMgr_timerSDP,
             connMgr_timerTCP,
             connMgr_timerAppl,
             connMgr_ConnectionLevel
            );
}

void connMgr_Mainfunction(void)
{
   /* count all the timers down */
   if (connMgr_timerEthLink>0) connMgr_timerEthLink--;
   if (connMgr_timerModemLocal>0) connMgr_timerModemLocal--;
   if (connMgr_timerModemRemote>0) connMgr_timerModemRemote--;
   if (connMgr_timerSlac>0) connMgr_timerSlac--;
   if (connMgr_timerSDP>0) connMgr_timerSDP--;
   if (connMgr_timerTCP>0) connMgr_timerTCP--;
   if (connMgr_timerAppl>0) connMgr_timerAppl--;

   /* Based on the timers, calculate the connectionLevel. */
   if      (connMgr_timerAppl>0)         connMgr_ConnectionLevel=CONNLEVEL_100_APPL_RUNNING;
   else if (connMgr_timerTCP>0)          connMgr_ConnectionLevel=CONNLEVEL_80_TCP_RUNNING;
   else if (connMgr_timerSDP>0)          connMgr_ConnectionLevel=CONNLEVEL_50_SDP_DONE;
   else if (connMgr_timerModemRemote>0)  connMgr_ConnectionLevel=CONNLEVEL_20_TWO_MODEMS_FOUND;
   else if (connMgr_timerSlac>0)         connMgr_ConnectionLevel=CONNLEVEL_15_SLAC_ONGOING;
   else if (connMgr_timerModemLocal>0)   connMgr_ConnectionLevel=CONNLEVEL_10_ONE_MODEM_FOUND;
   else if (connMgr_timerEthLink>0)      connMgr_ConnectionLevel=CONNLEVEL_5_ETH_LINK_PRESENT;
   else connMgr_ConnectionLevel=0;

   connMgr_timerEthLink = CONNMGR_TIMER_MAX; /* we have SPI, so just say ETH is up */

   if (connMgr_ConnectionLevelOld!=connMgr_ConnectionLevel)
   {
      if (Param::GetInt(Param::logging) & MOD_CONNMGR)
         printf("[%u] [CONNMGR] ConnectionLevel changed from %d to %d.\r\n", rtc_get_ms(), connMgr_ConnectionLevelOld, connMgr_ConnectionLevel);
      connMgr_ConnectionLevelOld = connMgr_ConnectionLevel;
   }
   if ((connMgr_cycles % 33)==0)
   {
      /* once per second */
      connMgr_printDebugInfos();
      if (connMgr_ConnectionLevel<CONNLEVEL_5_ETH_LINK_PRESENT)
      {
         publishStatus("Eth", "no link");
      }
   }
   connMgr_cycles++;
}

void connMgr_ModemFinderOk(uint8_t numberOfFoundModems)
{
   if (numberOfFoundModems>=1)
   {
      connMgr_timerModemLocal = CONNMGR_TIMER_MAX;
   }
   if (numberOfFoundModems>=2)
   {
      connMgr_timerModemRemote = CONNMGR_TIMER_MAX_10s; /* 10s for the slac sequence, to avoid too fast timeout */
   }
}

void connMgr_SlacOk(void)
{
   /* The SetKey was sent to the local modem. This leads to restart of the
   local modem, and potenially also for the remote modem. If both modems are up,
   they need additional time to pair. We need to be patient during this process. */
   connMgr_timerSlac = CONNMGR_TIMER_MAX_20s;
}

void connMgr_SdpOk(void)
{
   connMgr_timerSDP = CONNMGR_TIMER_MAX;
}

void connMgr_TcpOk(void)
{
   connMgr_timerTCP = CONNMGR_TIMER_MAX;
}

void connMgr_ApplOk(uint8_t timeout_in_seconds)
{
   connMgr_timerAppl = timeout_in_seconds * CONNMGR_CYCLES_PER_SECOND;
}


