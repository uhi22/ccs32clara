
/* Hardware Interface module */

#include "ccs32_globals.h"

void hardwareInterface_showOnDisplay(char* s1, char* s2, char* s3) {

}


void hardwareInterface_initDisplay(void) {

}

int hardwareInterface_sanityCheck() {
  return 0; /* 0 is OK */
}

uint16_t hwIf_simulatedSoc_0p1;

void hardwareInterface_simulatePreCharge(void) {
}

void hardwareInterface_simulateCharging(void) {
  if (hwIf_simulatedSoc_0p1<1000) {
    /* simulate increasing SOC */
    hwIf_simulatedSoc_0p1++;
  }
}

int16_t hardwareInterface_getInletVoltage(void) {
  return 219;
}

int16_t hardwareInterface_getAccuVoltage(void) {
  return 222;
}

int16_t hardwareInterface_getChargingTargetVoltage(void) {
  return 229;
}

int16_t hardwareInterface_getChargingTargetCurrent(void) {
  return 5;
}

uint8_t hardwareInterface_getSoc(void) {
  /* SOC in percent */
  return hwIf_simulatedSoc_0p1/10;
}

uint8_t hardwareInterface_getIsAccuFull(void) {
    return (hwIf_simulatedSoc_0p1/10)>95;
}

void hardwareInterface_setPowerRelayOn(void) {
  //digitalWrite(PIN_POWER_RELAIS, LOW); /* relais is low-active */
	/*
		    for (i=0; i<5; i++) {
		     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 1);
		     HAL_Delay(100);
		     HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, 0);
		     HAL_Delay(100);
		    }
	*/

}

void hardwareInterface_setPowerRelayOff(void) {
  //digitalWrite(PIN_POWER_RELAIS, HIGH); /* relais is low-active */
}

void hardwareInterface_setRelay2On(void) {
}

void hardwareInterface_setRelay2Off(void) {
}

void hardwareInterface_setStateB(void) {
  //digitalWrite(PIN_STATE_C, LOW);
}

void hardwareInterface_setStateC(void) {
  //digitalWrite(PIN_STATE_C, HIGH);
}

void hardwareInterface_resetSimulation(void) {
    hwIf_simulatedSoc_0p1 = 200; /* 20% */
}
