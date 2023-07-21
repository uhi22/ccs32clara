
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
	HAL_GPIO_WritePin(OUT_CONTACTOR_CONTROL1_GPIO_Port, OUT_CONTACTOR_CONTROL1_Pin, 1);
}

void hardwareInterface_setPowerRelayOff(void) {
	HAL_GPIO_WritePin(OUT_CONTACTOR_CONTROL1_GPIO_Port, OUT_CONTACTOR_CONTROL1_Pin, 0);
}

void hardwareInterface_setRelay2On(void) {
	HAL_GPIO_WritePin(OUT_CONTACTOR_CONTROL2_GPIO_Port, OUT_CONTACTOR_CONTROL2_Pin, 1);
}

void hardwareInterface_setRelay2Off(void) {
	HAL_GPIO_WritePin(OUT_CONTACTOR_CONTROL2_GPIO_Port, OUT_CONTACTOR_CONTROL2_Pin, 0);
}

void hardwareInterface_setStateB(void) {
	HAL_GPIO_WritePin(OUT_STATE_C_CONTROL_GPIO_Port, OUT_STATE_C_CONTROL_Pin, 0);
}

void hardwareInterface_setStateC(void) {
	HAL_GPIO_WritePin(OUT_STATE_C_CONTROL_GPIO_Port, OUT_STATE_C_CONTROL_Pin, 1);
}

void hardwareInterface_triggerConnectorLocking(void) {
  /* todo */
}

void hardwareInterface_triggerConnectorUnlocking(void) {
  /* todo */
}

uint8_t hardwareInterface_isConnectorLocked(void) {
  /* todo */
  return 1;
}

uint8_t hardwareInterface_getPowerRelayConfirmation(void) {
 /* todo */
 return 1;
}

void hardwareInterface_resetSimulation(void) {
    hwIf_simulatedSoc_0p1 = 200; /* 20% */
}


void hardwareInterface_cyclic(void) {
	snprintf(strTmp, 100, "CH_1: %d, CH_2: %d, CH_3: %d, CH_4: %d, CH_5: %d",
			adc_dma_result[0], adc_dma_result[1], adc_dma_result[2], adc_dma_result[3], adc_dma_result[4]);
	addToTrace(strTmp);
}
