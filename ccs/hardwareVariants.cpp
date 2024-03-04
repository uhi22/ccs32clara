
/* hardwareVariants  */

/* This module handles the detection of hardware variants.
   The hardware variant detection works with a voltage divider between 3.3V and ground. Each
   variant has a certain ratio between the high side and the low side resistor.

   Details here: https://openinverter.org/wiki/Fully_Open_CCS_Charge_Controller_(FOCCCI)
*/

#include "ccs32_globals.h"


#define MAX_ADC_VALUE 4095 /* we have 12 bit ADC resolution */

/* The AD values for each variant */
#define ADC_FOR_VERSION_4_2 718
#define ADC_FOR_VERSION_4_3 833
#define ADC_FOR_VERSION_4_4 991

uint8_t isNear(uint16_t x, uint16_t y) {
    #define TOLERANCE 30 /* ADC steps in each direction */
    return ((x+TOLERANCE)>y) && (x<(y+TOLERANCE));
}

void hw_evaluateHardwareVariants(void) {
    uint8_t channels[1] = { 2 };
    uint16_t adc, nHardwareVariant;

    adc_set_injected_sequence(ADC1, sizeof(channels), channels);
    adc_set_sample_time(ADC1, channels[0], ADC_SMPR_SMP_239DOT5CYC);
    adc_enable_external_trigger_injected(ADC1, ADC_CR2_JEXTSEL_JSWSTART);
    adc_start_conversion_injected(ADC1);
    while (!adc_eoc_injected(ADC1));
    adc = adc_read_injected(ADC1, 1);

    /* Provide the raw AD value (0 to 4095) for analysis purposes. */
    Param::SetInt(Param::AdcHwVariant, adc);

    /* Decide about the hardware variant */
    nHardwareVariant = 999; /* mark hardware variant as "unknown" in case no valid range is detected */
    if (isNear(adc, ADC_FOR_VERSION_4_2)) nHardwareVariant = 4002;
    if (isNear(adc, ADC_FOR_VERSION_4_3)) nHardwareVariant = 4003;
    if (isNear(adc, ADC_FOR_VERSION_4_4)) nHardwareVariant = 4004;

    Param::SetInt(Param::HardwareVariant, nHardwareVariant);

}

