
/* Pushbutton evaluation */

#include "ccs32_globals.h"

/* public interface */
uint16_t pushbutton_tButtonReleaseTime;
uint16_t pushbutton_tButtonPressTime;

uint8_t pushbutton_nNumberOfButtonPresses;
uint8_t pushbutton_buttonSeriesCounter;
uint16_t pushbutton_accumulatedButtonDigits;

/* local defines and variables */
#define PUSHBUTTON_NUMBER_OF_ENTRIES 4
uint8_t blButtonPressedOld;
uint8_t buttonSeriesEntries[PUSHBUTTON_NUMBER_OF_ENTRIES];

/* functions */

void pushbutton_processPushButtonSeries(void) {
  uint8_t i;
  uint16_t tmp16;
  for (i=0; i<PUSHBUTTON_NUMBER_OF_ENTRIES-1; i++) {
	  buttonSeriesEntries[i]=buttonSeriesEntries[i+1];
  }
  buttonSeriesEntries[PUSHBUTTON_NUMBER_OF_ENTRIES-1]=pushbutton_nNumberOfButtonPresses;
  tmp16 = buttonSeriesEntries[0];
  tmp16*=10;
  tmp16 += buttonSeriesEntries[1];
  tmp16*=10;
  tmp16 += buttonSeriesEntries[2];
  tmp16*=10;
  tmp16 += buttonSeriesEntries[3];
  pushbutton_buttonSeriesCounter++;
  if (pushbutton_buttonSeriesCounter==PUSHBUTTON_NUMBER_OF_ENTRIES) {
	  pushbutton_accumulatedButtonDigits = tmp16;
  }
}

void pushbutton_handlePushbutton(void) {
	uint8_t blButtonPressed;
	uint8_t i;
	//debugvalue = rawAdValues[MY_ADC_CHANNEL_PUSHBUTTON];
	blButtonPressed = rawAdValues[MY_ADC_CHANNEL_PUSHBUTTON]<1000;
	if (blButtonPressed) {
		if (!blButtonPressedOld) {
			pushbutton_tButtonReleaseTime=0;
			pushbutton_nNumberOfButtonPresses++;
			pushbutton_tButtonPressTime=0;
		}
		if (pushbutton_tButtonPressTime<PUSHBUTTON_CYCLES_PER_SECOND*60) pushbutton_tButtonPressTime++;
	} else {
        /* button is no pressed */
		if (pushbutton_tButtonReleaseTime<PUSHBUTTON_CYCLES_PER_SECOND*60) pushbutton_tButtonReleaseTime++;
		if (pushbutton_tButtonReleaseTime==PUSHBUTTON_CYCLES_PER_SECOND/2) {
			/* not pressed for 0.5s -> evaluate the series */
			//debugvalue=pushbutton_nNumberOfButtonPresses;
			pushbutton_processPushButtonSeries();
			pushbutton_nNumberOfButtonPresses=0;
		}
		if (pushbutton_tButtonReleaseTime==PUSHBUTTON_CYCLES_PER_SECOND*5) {
			/* 5s not pressed -> reset the series */
			for (i=0; i<PUSHBUTTON_NUMBER_OF_ENTRIES; i++) {
				buttonSeriesEntries[i]=0;
				pushbutton_buttonSeriesCounter=0;
				pushbutton_accumulatedButtonDigits = 0;
				pushbutton_tButtonPressTime=0;
			}
		}
	}
	blButtonPressedOld = blButtonPressed;
}
