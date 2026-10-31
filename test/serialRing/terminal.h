/* Host test (test/serialRing): the part of libopeninv's Terminal that RingTerminal needs. */

#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

class IPutChar
{
public:
   virtual void PutChar(char c) = 0;
   virtual ~IPutChar() {}
};

class Terminal;

typedef struct
{
   char const* cmd;
   void (*CmdFunc)(Terminal*, char*);
} TERM_CMD;

class Terminal : public IPutChar
{
public:
   Terminal(uint32_t usart, const TERM_CMD* commands) { (void)usart; (void)commands; }
   void PutChar(char c) { (void)c; }
   void SendBinary(const uint8_t* data, uint32_t length); /* in ring_test.cpp: Terminal's own send, simulated */
   void Run(); /* in ring_test.cpp: sends the reply the test has queued, if any */
};

#endif
