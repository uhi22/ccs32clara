/* Non-blocking serial output: see serialRing.cpp. */

#ifndef SERIALRING_H
#define SERIALRING_H

#include <stdint.h>
#include "terminal.h"

/* The serial terminal, with output that never blocks: every character printed goes, a whole line at a time, into a
   ring buffer that DMA sends to the UART. libopeninv's printf and the terminal commands reach it through PutChar. */
class RingTerminal : public Terminal
{
public:
   RingTerminal(uint32_t usart, const TERM_CMD* commands);
   void PutChar(char c);
   /* Terminal::Run, with the ring held back from interrupt context while it runs (see the ring's comment). */
   void Run();
};

/* Every 100 ms: once a second, if lines were dropped because the ring was full, a line saying how many. */
void serialRing_cyclic100ms(void);
/* Lines dropped because the ring was full, since power-on. */
uint32_t serialRing_droppedLines(void);

#endif
