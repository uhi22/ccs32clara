
/* Hardware Interface module */

#include "ccs32_globals.h"

uint16_t hwIf_pwmLock1_64k;

void hardwareInterface_showOnDisplay(char* s1, char* s2, char* s3) {

}


void hardwareInterface_initDisplay(void) {

}

int hardwareInterface_sanityCheck() {
  return 0; /* 0 is OK */
}

uint16_t hwIf_simulatedSoc_0p01;

void hardwareInterface_simulatePreCharge(void) {
}

void hardwareInterface_simulateCharging(void) {
  if (hwIf_simulatedSoc_0p01<10000) {
    /* simulate increasing SOC */
    hwIf_simulatedSoc_0p01++;
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
  return hwIf_simulatedSoc_0p01/100;
}

uint8_t hardwareInterface_getIsAccuFull(void) {
    return (hwIf_simulatedSoc_0p01/100)>95;
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
    hwIf_simulatedSoc_0p01 = 2000; /* 20% */
}


void hardwareInterface_measureCpPwm(void) {
	/* We want to measure the PWM ratio on the CP pin.
	 * But the F103's input capture does not support capturing
	 * both edges, only either rising or falling edge.
	 * On https://community.st.com/t5/stm32-mcu-products/stm32f103rb-timers-input-capture-both-edges/m-p/516376/highlight/true#M188076
	 * they say, that it is possible to configure two input capture channels
	 * for one single pin, so we can use one for rising and one for falling edge.
	 *
	 * The PWM measuring feature is explained in the STM32F103 reference manual
	 * https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
	 * in chapter 15.3.6 PWM input mode.
	 */

	/* PWM is on PA15
	 *
	 */
}

void hardwareInterface_cyclic(void) {
    /*Assign the new dutyCycle count to the capture compare register.*/
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, hwIf_pwmLock1_64k);
    hwIf_pwmLock1_64k+=10;

    hardwareInterface_measureCpPwm();
}

void hardwareInterface_init(void) {
	/* TIM3 configuration:
	 *  - clocked with 64MHz
	 *  - prescaler 8, which leads to division by 9, leads to 7.11MHz
	 *  - 65536 counter, leads to 108Hz or 9.2ms periode time.
	 */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1); /* PC6 lockdriver IN1 */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2); /* PC7 lockdriver IN2 */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3); /* PC8 contactor 1 */
	HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4); /* PC9 contactor 2 */
}
