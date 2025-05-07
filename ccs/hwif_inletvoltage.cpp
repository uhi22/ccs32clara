/* Hardware Interface for inlet voltage


 This module shall handle the calculation of the CCS inlet voltage.
 
 The inlet voltage may have two different sources:
 1. The EvsePresentVoltage reported by the charging station
 2. The voltage measured by Focccis analog hardware input

*/

#include "ccs32_globals.h"

uint8_t hwIf_isInletVoltageError = false;


/* cyclic runnable in 100ms. Reads the different inputs for inlet voltage and calculates
   the plausibilized spot value  */
void hwIf_handleInletVoltage(void) {
    float voltageDeviation;
    /* step 1: Take the ADC value and provide it as spot value */
    Param::SetFloat(Param::AdcInletVoltage, AnaIn::udc.Get());

    /* step 2: Calculate the hardware-based inlet voltage. No matter whether a measuring device
       is connected or selected. */
    /* Param::UdcOffset is the ADC value in case of zero high voltage.
        In case of the muehlpower board discussed in
        https://openinverter.org/forum/viewtopic.php?t=2474
        this is 1.42V on Foccci connector pin.
        Discussed here: https://github.com/uhi22/ccs32clara/issues/23.
        We allow to show negative voltages, which really works with the
        muehlpower board. The Foccci pin voltage in this case goes
        below the "idle" of 1.42V which is ~573 digits.
        Test table
        UDC   UFocci  AdcInletVoltage InletVoltageHw
          0V  1.42V      573dig        0V
        500V  4.8V      1890dig        500V
        The Gain parameter (UdcDivider) is (1890-573)/500V = 2.63 dig/V */
     Param::SetFloat(Param::InletVoltageHw,
                     (AnaIn::udc.Get() - Param::GetFloat(Param::UdcOffset)) / Param::GetFloat(Param::UdcDivider)
                    );

    /* step 3: depending on parametrization, take the input */
    switch (Param::GetInt(Param::InletVtgSrc)) {
        case IVSRC_CHARGER:
            /* The inlet voltage shall be taken from the charging stations EvsePresentVoltage. */
            Param::SetFixed(Param::InletVoltagePlaus, Param::Get(Param::EvsePresentVoltage));
            /* we have only one source, so we have no deviation and no error. */
            Param::SetFixed(Param::InletVoltageDeviation, 0);
            hwIf_isInletVoltageError = false;
            break;
        case IVSRC_ANAIN:
            /* if we have two values, the hardware value and the EvsePresentVoltage, we use the hardware value,
               and we can make a plausibilization between the two values. Discussed here: https://github.com/uhi22/ccs32clara/issues/52
            */
            Param::SetFixed(Param::InletVoltagePlaus, Param::Get(Param::InletVoltageHw));
            voltageDeviation = ABS(Param::Get(Param::InletVoltageHw) - Param::Get(Param::EvsePresentVoltage));
            Param::SetFixed(Param::InletVoltageDeviation, voltageDeviation); /* provide the deviation as spot value for analysis */
            if (voltageDeviation > Param::Get(Param::InletVoltageTolerance)) {
                /* bad case: we see a significant deviation between the EvsePresentVoltage and the hardware measured voltage.
                   This may NOT yet be a problem, because:
                   1. Not in all states we receive the EvsePresentVoltage. So we only should look at the error flag in
                      the states when it make sense.
                   2. There may be a time shift between the analog measured value and the values from the charging station.
                      So it may be senseful to add a debouning here, e.g. deviation longer than one or two seconds. */
                hwIf_isInletVoltageError = true;
            } else {
                /* good case: The EvsePresentVoltage fits to the hardware measured voltage */
                hwIf_isInletVoltageError = false;
            }
            break;
        default:
        case IVSRC_CAN:
            /* Do nothing, value received via CAN map */
            /* we have only one source, so we have no deviation and no error. */
            Param::SetFixed(Param::InletVoltageDeviation, 0);
            hwIf_isInletVoltageError = false;
            break;
    }
}


