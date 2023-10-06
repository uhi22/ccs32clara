

#include "ccs32_globals.h"

uint32_t canary1;
uint32_t currentTime;
uint32_t canary2;
uint32_t lastTime1s;
uint32_t canary3;
uint32_t lastTime100ms;
uint32_t lastTime200ms;
uint32_t lastTime30ms;
uint32_t lastTime10ms;
uint32_t canary4;
uint32_t nCycles30ms;
uint32_t canary5;


/**********************************************************/
/* The tasks */

/* This task runs each 10ms. */
void task10ms(void) {
	canbus_Mainfunction10ms();
}


/* This task runs each 30ms. */
void task30ms(void) {
  nCycles30ms++;
  spiQCA7000checkForReceivedData();  
  connMgr_Mainfunction(); /* ConnectionManager */
  modemFinder_Mainfunction();
  runSlacSequencer();
  runSdpStateMachine();
  tcp_Mainfunction();
  pevStateMachine_Mainfunction();

  //cyclicLcdUpdate();
  hardwareInterface_cyclic();
  myAdc_cyclic();
  pushbutton_handlePushbutton();
  sanityCheck("cyclic30ms");
}

/* This task runs each 100ms. */
void task100ms(void) {
	canbus_Mainfunction100ms();
}

/* This task runs each 200ms. */
void task200ms(void) {

}

/* This task runs once a second. */
void task1s(void) {
  //demoQCA7000(); 
  uint8_t blStop=0;
  if (canary1!=0x55AA55AA) {
	  sprintf(strTmp, "canary1 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary2!=0x55AA55AA) {
	  sprintf(strTmp, "canary2 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary3!=0x55AA55AA) {
	  sprintf(strTmp, "canary3 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary4!=0x55AA55AA) {
	  sprintf(strTmp, "canary4 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (canary5!=0x55AA55AA) {
	  sprintf(strTmp, "canary5 corrupted");
	  addToTrace(strTmp);
	  blStop=1;
  }
  if (blStop) {
	  while (1) { }
  }
}

void initMyScheduler(void) {
	addToTrace("=== ccs32clara started. Version 2023-07-18 ===");
	canary1 = 0x55AA55AA;
	canary2 = 0x55AA55AA;
	canary3 = 0x55AA55AA;
	canary4 = 0x55AA55AA;
	canary5 = 0x55AA55AA;
}

void runMyScheduler(void) {
  /* a simple scheduler which calls the cyclic tasks depending on system time */
  currentTime = HAL_GetTick(); /* HAL_GetTick() returns the time since startup in milliseconds */
  if (currentTime<lastTime30ms) {
	    sprintf(strTmp, "Error: current time jumped back. current %ld, last %ld", currentTime, lastTime30ms);
	    addToTrace(strTmp);
	    while (1) { }
  }
  if ((currentTime - lastTime10ms)>10) {
    lastTime10ms += 10;
    task10ms();
  }
  if ((currentTime - lastTime30ms)>30) {
    lastTime30ms += 30;
    //sprintf(strTmp, "current %ld, last %ld", currentTime, lastTime30ms);
    //addToTrace(strTmp);
    task30ms();
  }
  if ((currentTime - lastTime100ms)>100) {
    lastTime100ms += 100;
    task100ms();
  }
  if ((currentTime - lastTime200ms)>200) {
    lastTime200ms += 200;
    task200ms();
  }
  if ((currentTime - lastTime1s)>1000) {
    lastTime1s += 1000;
    task1s();
  }
}
