#ifndef PinMode_PRJ_H_INCLUDED
#define PinMode_PRJ_H_INCLUDED

#include "hwdefs.h"

/* Here you specify generic IO pins, i.e. digital input or outputs.
 * Inputs can be floating (INPUT_FLT), have a 30k pull-up (INPUT_PU)
 * or pull-down (INPUT_PD) or be an output (OUTPUT)
*/

#define DIG_IO_LIST \
    DIG_IO_ENTRY(test_in,     GPIOB, GPIO5,  PinMode::INPUT_FLT)   \
    DIG_IO_ENTRY(led_out,     GPIOC, GPIO12, PinMode::OUTPUT)      \

#endif // PinMode_PRJ_H_INCLUDED
