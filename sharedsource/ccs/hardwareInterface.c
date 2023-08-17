
/* Hardware Interface module */

#include "ccs32_globals.h"

uint16_t hwIf_pwmLock1_64k;
volatile uint32_t inputCaptureValueChannel1;
volatile uint32_t inputCaptureValueChannel2;
volatile uint32_t cpDuty_Percent, cpFrequency_Hz;
volatile uint32_t inputCaptureInterruptCounter;
volatile uint8_t cpDutyValidTimer;
#define CP_DUTY_VALID_TIMER_MAX 3 /* after 3 cycles with 30ms, we consider the CP connection lost, if
                                     we do not see PWM interrupts anymore */
#define APB2_TIMER_CLOCK_FREQUENCY_HZ 64000000u /* APB2 timer clock is 64 MHz */


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
	 *
	 * In
	 *  https://controllerstech.com/pwm-input-in-stm32/
	 * they explain the settings in the cubeIDE to have two timer channels combined to measure
	 * both edges from one pin.
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

    if (cpDutyValidTimer>0) {
    	/* we have a CP PWM interrupt seen not too long ago. Just count the timeout. */
    	cpDutyValidTimer--;
    } else {
    	/* no CP PWM interrupt. The timeout expired. We set the measured PWM and duty to zero. */
        cpDuty_Percent = 0;
        cpFrequency_Hz = 0;
    }
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

	/* The CP PWM measurement */
	/* https://controllerstech.com/pwm-input-in-stm32/ */
	/* Using two channels of TIM2 to measure duty cycle */
	HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);   // main channel
	HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2);   // indirect channel
}


void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
	inputCaptureInterruptCounter++;
	if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)  // If the interrupt is triggered by channel 1
	{
		// Read the IC value
		inputCaptureValueChannel1 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
		inputCaptureValueChannel2 = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
		if (inputCaptureValueChannel1 != 0)
		{
			// calculate the Duty Cycle
			cpDuty_Percent = (inputCaptureValueChannel2 *100)/inputCaptureValueChannel1;

			cpFrequency_Hz = APB2_TIMER_CLOCK_FREQUENCY_HZ/inputCaptureValueChannel1;
			cpDutyValidTimer = CP_DUTY_VALID_TIMER_MAX;

		}
	}
}
