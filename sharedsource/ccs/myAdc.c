
/* myAdc  */

/* This module reads analog values.

  The STM32 offers different methods to use the ADC.
  (A) ADC with DMA. This is offered as the best and simplest choice. In the CubeMX,
      you configure the ADC channels and the DMA, which transfers the converted
      values into memory. The ADC conversion and the transfer to the memory happen
      continuously in background, so that the application just needs to read the converted values
      for the memory. This looks very good on the first look.
      But: When only needed a small amount of samples (e.g. four channels, so the memory
      has four entries of uint16_t), the DMA causes a high CPU load due to fireing
      the interrupts for "Finished half way" and "Finished completely". We would not
      need to interrupts at all for the simple use case, but there seems to be no way to
      disable them in the CubeMX. The result is: When using just 4 channels,
      the interrupts are blocking the CPU completely, so that the main loop is not
      executed. Very bad.
      Workaround 1: Increase the memory buffer to 8 elements, so that the DMA interrupt
      rate is only the half. This is quite dirty. Lot of CPU time is wasted by the
      not necessary interrupts, and changes in the number of ADC channels always causes
      the question: How to configure the DMA buffer size, to reach a balance between
      wasted RAM and wasted run time?
  (B) ADC without DMA. The simple idea would be, to just tell the STM "start conversion
      for channel x", then wait (some microseconds) until sampling and conversion is done,
      then read the result. But with the STM, this is not such simple.
      The cubeMX needs to be configured to have all required AD channels, and for
      each channel a separate "Rank" is configured.
       ScanMode = yes
       ContinuousConversionMode = yes
       No DMA
       NumberOfConversions = number of channels
       For each rank, set the channel and the sampling time
      
      Then we need to patch the generated configuration code, so that it does not
      initialize the sequencer in the MX_ADC1_Init function.
      Change the hadc1.Init.NbrOfConversion = 1
      And, write own ADC_Select_Channelx functions, which set the channel, rank=1, samplingtime
      by calling HAL_ADC_ConfigChannel.
      To get a sample:
        ADC_Select_Channelx(); // The own function to select a channel
        HAL_ADC_Start(&hadc1);
        HAL_ADC_PollForConversion(&hadc1, HAL_ADC_TIMEOUT_1000MS);
        myAdValue = HAL_ADC_GetValue(&hadc1);
        HAL_ADC_Stop(&hadc1);
      Repeat this for all necessay channels.  
      Also this approach looks quite dirty: Patching the generated code and creating
      a lot of code for such a simple thing.
      Runtime measured: 138µs for 5 channels, means 28µs per channel.
         With prescaler ADC_CLOCK_SYNC_PCLK_DIV4 and sampling time ADC_SAMPLETIME_7CYCLES_5.
         (later changed to ADC_SAMPLETIME_71CYCLES_5)
      Running a higher clock or shorter sampling seems to make the measurements wrong.

      Reference for the concept: https://www.youtube.com/watch?v=5l-b6lsubBE
      
      
*/

#include "ccs32_globals.h"

#define HAL_ADC_TIMEOUT_1MS 1

/* For the F303, the values from data sheet are:
 * AvgSlope 4.0 to 4.6 mV/K
 * V25 1.34V to 1.52V at 25°C
 */
  //#define Avg_Slope 0.0043 /* volts per Kelvin */
  //#define V25 1.43 /* volts at 25°C */
/* other derivates may have other parameters (e.g. 0.7V) */
#ifdef CONTROLLER_TYPE_STM32F103
  /* https://www.st.com/resource/en/reference_manual/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
   * https://www.st.com/resource/en/datasheet/stm32f103RE.pdf
   */
  #define Avg_Slope 4300 /* microvolts per Kelvin */
  #define V25 1430 /* millivolts at 25°C */
  #include "stm32f1xx_ll_adc.h"
#endif


int16_t uCcsInlet_V = 0;
uint16_t rawAdValues[MYADC_NUMBER_OF_CHANNELS];
uint8_t myadc_cycleDivider;

void myAdc_SelectChannel_Temp1(void) {
  /** Configure Regular Channel
  */
#ifdef CONTROLLER_TYPE_STM32F303
  sConfig.Channel = ADC_CHANNEL_6;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
#else
  /* The F103 has other attributes of the ADC and other channel mapping */
  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_71CYCLES_5;
#endif

  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void myAdc_SelectChannel_Temp2(void) {
  /** Configure Regular Channel
  */
#ifdef CONTROLLER_TYPE_STM32F303
  sConfig.Channel = ADC_CHANNEL_7;
#else
  sConfig.Channel = ADC_CHANNEL_11;
#endif
  sConfig.Rank = ADC_REGULAR_RANK_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void myAdc_SelectChannel_Temp3(void) {
  /** Configure Regular Channel
  */
#ifdef CONTROLLER_TYPE_STM32F303
  sConfig.Channel = ADC_CHANNEL_8;
#else
  sConfig.Channel = ADC_CHANNEL_12;
#endif
  sConfig.Rank = ADC_REGULAR_RANK_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void myAdc_SelectChannel_DcVoltage(void) {
  /** Configure Regular Channel
  */
#ifdef CONTROLLER_TYPE_STM32F303
  sConfig.Channel = ADC_CHANNEL_9;
#else
  sConfig.Channel = ADC_CHANNEL_13;
#endif
  sConfig.Rank = ADC_REGULAR_RANK_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

void myAdc_SelectChannel_TEMP(void) {
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
}

uint16_t myAdc_analogRead(uint8_t channel) {
  uint16_t myAdValue;
  switch (channel) {
    case 0: myAdc_SelectChannel_Temp1(); /* The own function to select a channel */
  	  break;
    case 1: myAdc_SelectChannel_Temp2(); /* The own function to select a channel */
  	  break;
    case 2: myAdc_SelectChannel_Temp3(); /* The own function to select a channel */
  	  break;
    case 3: myAdc_SelectChannel_DcVoltage(); /* The own function to select a channel */
  	  break;
    case 4: myAdc_SelectChannel_TEMP(); /* The own function to select a channel */
  	  break;
  }

  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_ADC_TIMEOUT_1MS);
  myAdValue = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);
  return myAdValue;
}

void myAdc_measureCpuTemperature(void) {
  uint16_t myAdValue;
  myAdc_SelectChannel_TEMP(); /* The own function to select a channel */
  HAL_ADC_Start(&hadc1);
  HAL_ADC_PollForConversion(&hadc1, HAL_ADC_TIMEOUT_1MS);
  myAdValue = HAL_ADC_GetValue(&hadc1);
  HAL_ADC_Stop(&hadc1);

  //fCpuTemperature = (((3.3*myAdValue)/4095 - V25)/Avg_Slope)+25;
  #ifdef CONTROLLER_TYPE_STM32F103
  fCpuTemperature_Celsius = __LL_ADC_CALC_TEMPERATURE_TYP_PARAMS(
		  Avg_Slope,
		  V25,
		  25,
		  3300 /* mV reference voltage */,
		  myAdValue,
		  LL_ADC_RESOLUTION_12B);
  #endif
}


void myAdc_calculateDcVoltage(void) {
	int32_t tmp;
	/* In idle case (0V input), the muehlpower board provides 1.403V.
	 * This is divided by 47k and 22k, which results in
	 * 1.403V * 22k / (22k+47k) = 0.447V
	 * With a reference voltage of 3.31V and 12 bit resolution, the AD value
	 * for this case is 4096 * 0.447V / 3.31V = 553 digits.
	 * With the foccci board we measure ~551.
	 *
	 * In https://openinverter.org/forum/viewtopic.php?p=24613#p24613 we find
	 *   1.42V for 0V HV
	 *   4.8V for 500V HV
	 * This would be a scaling factor of 3.38V/500V. After dividing with 47k and 22k, we
	 * get 1,077V/500V, and with the ADC with 3.3V and 12 bit this is
	 * 1338 digits / 500V. This is 373mV per digit.
	 */
    #define DC_LSB_FOR_ZERO_DC_INPUT 552
    #define DC_MILLIVOLT_PER_LSB 373
	tmp = rawAdValues[3];
	tmp -= DC_LSB_FOR_ZERO_DC_INPUT; /* The ADC value for U_HV=0V */
    tmp *= DC_MILLIVOLT_PER_LSB;
    tmp /= 1000; /* millivolt to volt */
    uCcsInlet_V = tmp;
}

void myAdc_calibrate(void) {
	/* https://www.mikrocontroller.net/topic/390110
	 * The calibration must be called BEFORE HAL_ADC_Start() or after _Stop.
	 * Since we start and stop the ADC with each sample, this is fulfilled
	 * by calling the calibration once during startup.
	 *
	 * The
	 *    https://community.st.com/t5/embedded-software-mcus/how-to-calibrate-the-adcs-on-stm32-using-the-hal-library/td-p/441795
	 * refers to application note AN2834.
	 * https://www.st.com/resource/en/application_note/an2834-how-to-get-the-best-adc-accuracy-in-stm32-microcontrollers-stmicroelectronics.pdf
	 *
	 */
	HAL_ADCEx_Calibration_Start(&hadc1);
}

void myAdc_cyclic(void) {
  /* read all AD channels and store the result in global variables */
  uint8_t i;
  for (i=0; i<5; i++) {
    rawAdValues[i] = myAdc_analogRead(i);
  }
  myAdc_measureCpuTemperature();
  myAdc_calculateDcVoltage();
  temperatures_calculateTemperatures();
  myadc_cycleDivider++;
  if (myadc_cycleDivider>=2) {
	  myadc_cycleDivider = 0;
	  sprintf(strTmp, "raw AD %d %d %d %d %d, uInlet %d V, temps %3.0f %3.0f %3.0f celsius, CPU %3.0f celsius",
				rawAdValues[0],
				rawAdValues[1],
				rawAdValues[2],
				rawAdValues[3],
				rawAdValues[4],
				uCcsInlet_V,
				temperatureChannel_1_celsius,
				temperatureChannel_2_celsius,
				temperatureChannel_3_celsius,
				fCpuTemperature_Celsius
				);
	  addToTrace(strTmp);
  }
}

void myAdc_demo(void) {
	uint8_t i;
	uint32_t loops;
	uint32_t ticksBefore, ticksAfter, ticksDelta;
	/* runtime measurement for the AD conversion */
	ticksBefore = uwTick;
	for (loops=0; loops<1000; loops++) { /* run 1000 times over the loop, to get a measurable time */
		for (i=0; i<5; i++) {
			rawAdValues[i] = myAdc_analogRead(i);
		}
	}
	ticksAfter = uwTick;
	ticksDelta = ticksAfter - ticksBefore; /* the number of milliseconds for the big loop */
	sprintf(strTmp, "run time (ms) %ld", ticksDelta);
    addToTrace(strTmp);

	sprintf(strTmp, "raw AD %d %d %d %d %d",
			rawAdValues[0],
			rawAdValues[1],
			rawAdValues[2],
			rawAdValues[3],
			rawAdValues[4]
			);
    addToTrace(strTmp);

    myAdc_measureCpuTemperature();
    sprintf(strTmp, "CPU temperature %f", fCpuTemperature);
    addToTrace(strTmp);
}
