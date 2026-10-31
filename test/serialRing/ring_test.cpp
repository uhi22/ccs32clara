/* Host test of the non-blocking serial output (ccs/serialRing.cpp). Runs on a PC: make -C test/serialRing run

   ccs/serialRing.cpp is compiled as it is, with the hardware simulated (serialRing_hosttest.h):
   - DMA2 channel 5 (EN, CNDTR, CMAR, TCIF, TCIE) takes bytes at a random pace, as the UART does, from the ring or
     from libopeninv Terminal's own output buffers, whichever was programmed. Like the STM32, it ignores CNDTR and CMAR
     writes while enabled, and restarts from CMAR when enabled. The ring's code writing them while enabled fails the
     test. TCIF is set when the count reaches 0.
   - Its interrupt runs whenever TCIF and TCIE are set and BASEPRI doesn't hold it off; one that keeps firing without
     end fails the test.
   - The main loop prints lines one character at a time: short lines, json-sized replies (10 kB on one line), and now
     and then one of Terminal's own sends (its reply to an unknown command, "fastuart", "echo"), done the way
     libopeninv's Terminal::SendCurrentBuffer does it: wait for TCIF (unless it is the first send), disable the
     channel, program it, clear TCIF, enable it. A wait that never ends fails the test.
   - The scheduler's interrupt (which the main loop can't interrupt) prints whole lines, marked so the test can tell
     them apart, and runs the 100 ms task: between any two of the main loop's characters, while the main loop waits
     for room, and between the steps of Terminal's send after its wait.

   Checks, over millions of steps and thousands of trips round the ring:
   - What the DMA read from the ring is exactly what was put into the ring, in order, each byte once.
   - With the interrupts' lines and the drop reports taken out, the output is exactly what the main loop printed and
     Terminal sent, in order: nothing lost, nothing added (no line breaks the caller didn't write).
   - Terminal's sends never land inside a main-loop line, nor inside an interrupt's line unless that line is longer
     than LINE_LEN (with long lines on: its pieces go into the ring one by one, so the ring can run dry in between).
   - No interrupt's output lands inside a main-loop piece while it is being put into the ring.
   - Every interrupt line either reached the UART whole or was counted as dropped, and the drop reports add up.
   - Neither Terminal's send nor the main loop waits for ever, and the DMA interrupt doesn't keep firing.
   The json replies are printed with the ring held back from interrupt context, as from inside Terminal::Run, and
   Terminal's own sends come from Terminal::Run.

   usage: ring_test [seed] [operations] [long] */

#include "../../ccs/serialRing.cpp"

#include <random>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

/* --- the simulation --- */

static std::mt19937 rng(1);
static uint32_t simBasepri;
static uint32_t simIpsr;
static bool simInIsr;
static uint32_t simTicks;
static int uartMax = 3; /* bytes the UART may take per step */

static struct
{
   bool en, tcie, tcif;
   uint32_t cmar;
   uint16_t cndtr;
   uint32_t cur; /* the address the next byte comes from */
} dma;

static char termBuf[2][128]; /* libopeninv Terminal's outBuf[2][bufSize] */
static int termCur;
static bool termFirstSend = true;

static std::string wire;         /* what the UART sent */
static std::string wireFromRing; /* ...for each byte of it, 1 if it came from the ring, 0 if from Terminal's buffers */
static std::string ringOut;      /* ...of it, what the DMA read from the ring */
static std::string ringExpected; /* what was put into the ring */
static std::string mainOrder;    /* what the main loop printed and Terminal sent, in order */
static unsigned long failures;

/* coverage */
static unsigned long isrRuns, isrHeldOff, writesEndingAtWrap, dmaStarts, termSends, termWaitSteps, mainWaits,
   jsonReplies;

static void fail(const char* what)
{
   if (failures++ < 10)
      printf("FAIL: %s\n", what);
}

static bool isrAllowed(void)
{
   return dma.tcif && dma.tcie && !simInIsr && (simBasepri == 0 || simBasepri > LOG_MASK_LEVEL);
}

static void maybeIsr(void)
{
   int runs = 0;
   if (dma.tcif && dma.tcie && !simInIsr && !isrAllowed())
      isrHeldOff++;
   while (isrAllowed())
   {
      uint32_t savedIpsr = simIpsr;
      if (++runs > 1000)
      {
         fail("the DMA interrupt keeps firing");
         dma.tcie = false;
         break;
      }
      simInIsr = true;
      simIpsr = 16 + NVIC_DMA2_CHANNEL4_5_IRQ;
      isrRuns++;
      dma2_channel4_5_isr();
      simIpsr = savedIpsr;
      simInIsr = false;
   }
}

/* The byte at a DMA address: in the ring (fromRing) or in Terminal's buffers. */
static char readMemory(uint32_t addr, bool* fromRing)
{
   uint32_t ringBase = (uint32_t)(uintptr_t)ring, termBase = (uint32_t)(uintptr_t)termBuf;
   if (addr - ringBase < LOG_RING)
   {
      *fromRing = true;
      return (char)ring[addr - ringBase];
   }
   if (addr - termBase < sizeof(termBuf))
   {
      *fromRing = false;
      return ((char*)termBuf)[addr - termBase];
   }
   fail("the DMA reads outside the ring and Terminal's buffers");
   dma.en = false;
   *fromRing = false;
   return 0;
}

/* The UART takes a few bytes; then the interrupt runs if it may. */
static void hwStep(void)
{
   int k = (int)(rng() % (unsigned)(uartMax + 1));

   simTicks += 1 + rng() % 40;
   while (k-- > 0 && dma.en && dma.cndtr > 0)
   {
      bool fromRing;
      char c = readMemory(dma.cur, &fromRing);
      if (!dma.en)
         break;
      wire += c;
      wireFromRing += (char)fromRing;
      if (fromRing)
         ringOut += c;
      dma.cur++;
      if (--dma.cndtr == 0)
         dma.tcif = true;
   }
   maybeIsr();
}

/* --- what the ring's code calls --- */

void dma_disable_channel(uint32_t, uint8_t)
{
   hwStep();
   dma.en = false;
}

void dma_enable_channel(uint32_t, uint8_t)
{
   dma.en = true;
   dma.cur = dma.cmar;
   dmaStarts++;
   hwStep();
}

void dma_set_memory_address(uint32_t, uint8_t, uint32_t address)
{
   if (dma.en)
      fail("the ring writes the DMA address while the channel is enabled");
   else
      dma.cmar = address;
}

void dma_set_number_of_data(uint32_t, uint8_t, uint16_t number)
{
   if (dma.en)
      fail("the ring writes the DMA count while the channel is enabled");
   if (number == 0)
      fail("DMA started with a count of 0");
   if (!dma.en)
      dma.cndtr = number;
}

void dma_clear_interrupt_flags(uint32_t, uint8_t, uint32_t interrupts)
{
   if (interrupts & DMA_TCIF)
      dma.tcif = false;
}

void dma_enable_transfer_complete_interrupt(uint32_t, uint8_t)
{
   dma.tcie = true;
}

void dma_disable_transfer_complete_interrupt(uint32_t, uint8_t)
{
   dma.tcie = false;
}

bool dma_get_interrupt_flag(uint32_t, uint8_t, uint32_t flags)
{
   return (flags & DMA_TCIF) && dma.tcif;
}

uint8_t dmaEnabled(void)
{
   return dma.en;
}

uint16_t dmaRemaining(void)
{
   return dma.cndtr;
}

uint32_t dmaMemoryAddress(void)
{
   return dma.cmar;
}

void nvic_set_priority(uint8_t, uint8_t) {}
void nvic_enable_irq(uint8_t) {}

uint32_t rtc_get_ms(void)
{
   return simTicks / 100;
}

uint32_t maskLog(void)
{
   uint32_t old = simBasepri;
   hwStep();
   if (simBasepri == 0 || simBasepri > LOG_MASK_LEVEL)
      simBasepri = LOG_MASK_LEVEL;
   return old;
}

void unmaskLog(uint32_t old)
{
   simBasepri = old;
   maybeIsr();
   hwStep();
}

uint32_t currentIpsr(void)
{
   return simIpsr;
}

uint32_t interruptsDisabled(void)
{
   return 0;
}

/* --- writing, and recording what was put into the ring --- */

static int captureDepth; /* an interrupt's write inside another write is read with the outer one */

template <class F> static void committed(F f)
{
   if (captureDepth > 0)
   {
      f();
      return;
   }
   captureDepth++;
   uint16_t h0 = head;
   f();
   captureDepth--;
   uint16_t h1 = head;
   /* (what went into the ring is recorded byte by byte, by hostRecordByte: a write that waits for room can see the
      ring go round more than once) */
   if (h1 == 0 && h0 != 0)
      writesEndingAtWrap++;
}

static RingTerminal* term;
static bool allowLongLines;
static unsigned long preemptions;
static unsigned long isrLinesTried;
static unsigned long waitSteps; /* steps the main loop has waited for its current piece */

static std::string randomText(int n, const char* chars)
{
   size_t k = strlen(chars);
   std::string s;
   for (int i = 0; i < n; i++)
      s += chars[rng() % k];
   return s;
}

static const char* const textChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 =.,:-_/()";
static const char* const jsonChars = "abcdefghijklmnopqrstuvwxyz0123456789 \"{}[],:.-";

/* An interrupt's line, marked with \x01 so that the check can take it out; long lines only when allowed. */
static std::string isrLineText(bool allowLong)
{
   int maxLen = allowLong && rng() % 20 == 0 ? 400 : LINE_LEN - 12;
   return "\x01[" + std::to_string(rng() % 100000) + "] " + randomText(rng() % (maxLen - 12), textChars) + "\r\n";
}

static void isrAction(void)
{
   if (rng() % 100 < 80)
   {
      std::string s = isrLineText(allowLongLines);
      isrLinesTried++;
      for (char c : s)
         committed([&] { term->PutChar(c); });
   }
   else
   {
      committed([&] { serialRing_cyclic100ms(); });
   }
}

/* The scheduler's interrupt, now (the caller has checked that the masking allows it). */
static void runSchedulerIsr(void)
{
   if (term == 0)
      return; /* not yet: RingTerminal's constructor is still running */
   uint32_t saved = simIpsr;
   simIpsr = 16 + 30; /* TIM4 */
   isrAction();
   simIpsr = saved;
}

/* Before each byte goes into the ring: time passes, and the scheduler's interrupt (priority 0xE0) may interrupt the
   main loop, unless BASEPRI holds it off. Correct code holds it off while it writes, so this only runs the DMA
   interrupt (in hwStep); a missing mask would let the interrupt's output land inside the main loop's. */
void hostPreemptionPoint(void)
{
   hwStep();
   if (simIpsr == 0 && (simBasepri == 0 || simBasepri > 0xE0) && rng() % 32 == 0)
   {
      preemptions++;
      runSchedulerIsr();
   }
}

/* The main loop waits for room: the UART runs, and the scheduler's interrupt may come. */
void hostWaitForRoom(void)
{
   mainWaits++;
   if (++waitSteps > 2000000)
   {
      fail("the main loop waits for room for ever");
      head = tail; /* give up on this run's ring, to end the test */
      return;
   }
   hwStep();
   if (rng() % 64 == 0)
      runSchedulerIsr();
}

void hostRecordByte(uint8_t c)
{
   ringExpected += (char)c;
}

static void mainPutChar(char c)
{
   waitSteps = 0;
   committed([&] { term->PutChar(c); });
}

/* libopeninv's Terminal::SendBinary / Send, as SendCurrentBuffer does it, from the main loop. */
static void terminalSend(const std::string& s)
{
   unsigned long spins = 0;
   size_t n = s.size() < sizeof(termBuf[0]) ? s.size() : sizeof(termBuf[0]);

   memcpy(termBuf[termCur], s.data(), n);
   termSends++;
   while (!dma.tcif && !termFirstSend)
   {
      termWaitSteps++;
      if (++spins > 2000000)
      {
         fail("Terminal's own send waits for TCIF for ever");
         return;
      }
      hwStep();
      if (rng() % 64 == 0)
         runSchedulerIsr();
   }
   /* disable, program, clear TCIF, enable; the scheduler's interrupt may come between the steps */
   dma.en = false;
   if (rng() % 3 == 0)
      runSchedulerIsr();
   if (!dma.en)
      dma.cndtr = (uint16_t)n;
   if (rng() % 3 == 0)
      runSchedulerIsr();
   if (!dma.en)
      dma.cmar = (uint32_t)(uintptr_t)termBuf[termCur];
   if (rng() % 3 == 0)
      runSchedulerIsr();
   dma.tcif = false;
   if (rng() % 3 == 0)
      runSchedulerIsr();
   dma.en = true;
   dma.cur = dma.cmar;
   termCur = !termCur;
   termFirstSend = false;
   mainOrder += s.substr(0, n);
   hwStep();
}

void Terminal::SendBinary(const uint8_t* data, uint32_t length)
{
   terminalSend(std::string((const char*)data, length));
}

static std::string queuedReply; /* a reply Terminal::Run is to send */

void Terminal::Run()
{
   if (!queuedReply.empty())
   {
      std::string s = queuedReply;
      queuedReply.clear();
      SendBinary((const uint8_t*)s.data(), (uint32_t)s.size());
   }
}

/* --- the run --- */

/* What the main loop printed and Terminal sent, as it reached the UART: the ring's bytes with the interrupts' lines
   (\x01 ... \r\n) and the drop reports taken out, and Terminal's sends where they came between the ring's bytes. The
   reports' counts go into reported. A send of Terminal's that started inside a main-loop line is counted in
   inMainLine, inside an interrupt's line or a report in inIsrLine. */
static std::string mainOnly(unsigned long* isrLinesGot, unsigned long* reported, unsigned long* inMainLine,
                            unsigned long* inIsrLine)
{
   std::string r;                     /* the ring's bytes */
   std::vector<size_t> sendAt;        /* for each run of Terminal's bytes, where in r it came */
   std::vector<std::string> sendText; /* ...and the bytes */
   for (size_t i = 0; i < wire.size(); i++)
   {
      if (wireFromRing[i])
      {
         r += wire[i];
         continue;
      }
      if (i == 0 || wireFromRing[i - 1])
      {
         sendAt.push_back(r.size());
         sendText.push_back("");
      }
      sendText.back() += wire[i];
   }

   std::string out;
   size_t pos = 0, next = 0;
   *isrLinesGot = *reported = *inMainLine = *inIsrLine = 0;
   for (;;)
   {
      /* Terminal's sends that reached the UART before the ring's byte at pos */
      while (next < sendAt.size() && sendAt[next] <= pos)
      {
         if (!out.empty() && out.back() != '\n')
            (*inMainLine)++;
         out += sendText[next++];
      }
      if (pos >= r.size())
         break;
      size_t skipTo = pos;
      if (r[pos] == '\x01')
      {
         size_t e = r.find("\r\n", pos);
         if (e == std::string::npos)
         {
            fail("an interrupt's line doesn't end");
            break;
         }
         (*isrLinesGot)++;
         skipTo = e + 2;
      }
      else if (r[pos] == '[')
      {
         /* exactly "[<digits>] Serial output: <digits> lines dropped (ring full)\r\n" (json text has '[' too) */
         size_t q = pos + 1;
         while (q < r.size() && r[q] >= '0' && r[q] <= '9')
            q++;
         static const char mid[] = "] Serial output: ";
         static const char end[] = " lines dropped (ring full)\r\n";
         if (q > pos + 1 && r.compare(q, sizeof(mid) - 1, mid) == 0)
         {
            size_t d = q + sizeof(mid) - 1, e = d;
            while (e < r.size() && r[e] >= '0' && r[e] <= '9')
               e++;
            if (e > d && r.compare(e, sizeof(end) - 1, end) == 0)
            {
               *reported += strtoul(r.substr(d, e - d).c_str(), 0, 10);
               skipTo = e + sizeof(end) - 1;
            }
         }
      }
      if (skipTo == pos)
      {
         out += r[pos++];
         continue;
      }
      /* sends that came inside the interrupt's line or the report: they go out here, as nothing of the main loop's
         is inside it */
      while (next < sendAt.size() && sendAt[next] < skipTo)
      {
         if (sendAt[next] > pos)
            (*inIsrLine)++;
         else if (!out.empty() && out.back() != '\n')
            (*inMainLine)++;
         out += sendText[next++];
      }
      pos = skipTo;
   }
   return out;
}

static int run(unsigned seed, unsigned long ops)
{
   int isrShare = 40; /* percent of steps the scheduler's interrupt writes something; the rest the UART runs */
   rng.seed(seed);
   std::string pending;
   size_t pendingPos = 0;

   for (unsigned long i = 0; i < ops; i++)
   {
      if (i % 50000 == 0)
      {
         static const int paces[] = { 0, 1, 1, 2, 3, 6, 12 };
         static const int shares[] = { 2, 5, 10, 40 };
         uartMax = paces[rng() % 7];
         isrShare = shares[rng() % 4];
      }
      int r = rng() % 100;
      if (r < 45)
      {
         /* the main loop prints its next character, or between lines one of Terminal's own sends */
         if (pendingPos >= pending.size())
         {
            int what = rng() % 400;
            if (what < 8)
            {
               static const char* const replies[] = { "Unknown command sequence\r\n", "\nOK\r\n",
                                                      "Baud rate now 921600\r\n", "Echo on\r\n" };
               if (uartMax == 0)
                  uartMax = 1; /* Terminal's wait needs the UART to run */
               queuedReply = replies[rng() % 4];
               waitSteps = 0;
               term->Run(); /* Terminal sends it from its Run, as it does */
               continue;
            }
            if (what == 8)
            {
               pending = "{" + randomText(10000, jsonChars) + "}\r\n"; /* a "json" reply: 10 kB on one line */
               jsonReplies++;
               terminalBusy = 1; /* printed by a command, from inside Terminal::Run */
               if (uartMax == 0)
                  uartMax = 1; /* the main loop waits for room: the UART must run */
            }
            else
            {
               pending = randomText(rng() % (LINE_LEN - 2), textChars) + "\r\n";
            }
            pendingPos = 0;
            mainOrder += pending;
         }
         char c = pending[pendingPos++];
         simIpsr = 0;
         if (uartMax == 0)
            uartMax = 1; /* the main loop may have to wait for room */
         mainPutChar(c);
         if (terminalBusy && pendingPos >= pending.size())
         {
            /* the command is done: Terminal::Run returns, and the ring is started again */
            terminalBusy = 0;
            uint32_t old = maskLog();
            startDma();
            unmaskLog(old);
         }
      }
      else if (r < 45 + isrShare)
      {
         runSchedulerIsr();
      }
      else
      {
         hwStep();
      }
   }
   /* finish the main loop's line, then let the UART send everything */
   simIpsr = 0;
   uartMax = 8;
   while (pendingPos < pending.size())
      mainPutChar(pending[pendingPos++]);
   if (terminalBusy)
   {
      /* a json reply was under way: its command ends, Terminal::Run returns */
      terminalBusy = 0;
      uint32_t old = maskLog();
      startDma();
      unmaskLog(old);
   }
   for (int round = 0; round < 3; round++)
   {
      for (int i = 0; i < 10000000 && (head != tail || dma.cndtr != 0); i++)
         hwStep();
      simIpsr = 16 + 30;
      for (int i = 0; i < 10; i++)
         committed([&] { serialRing_cyclic100ms(); }); /* the last drop report, and a kick */
      simIpsr = 0;
   }
   for (int i = 0; i < 10000000 && (head != tail || dma.cndtr != 0); i++)
      hwStep();

   if (ringOut != ringExpected)
   {
      size_t k = 0;
      while (k < ringOut.size() && k < ringExpected.size() && ringOut[k] == ringExpected[k])
         k++;
      printf("from the ring %zu bytes, into it %zu bytes, first difference at byte %zu\n", ringOut.size(),
             ringExpected.size(), k);
      fail("what the DMA read from the ring differs from what was put into it");
   }
   unsigned long isrLinesGot, reported, sendsInMainLine, sendsInIsrLine;
   std::string mainGot = mainOnly(&isrLinesGot, &reported, &sendsInMainLine, &sendsInIsrLine);
   if (mainGot != mainOrder)
   {
      size_t k = 0;
      while (k < mainGot.size() && k < mainOrder.size() && mainGot[k] == mainOrder[k])
         k++;
      printf("main-loop output %zu bytes, printed %zu bytes, first difference at byte %zu\n", mainGot.size(),
             mainOrder.size(), k);
      printf("  printed: ...%s...\n", mainOrder.substr(k > 60 ? k - 60 : 0, 160).c_str());
      printf("  got:     ...%s...\n", mainGot.substr(k > 60 ? k - 60 : 0, 160).c_str());
      fail("the main loop's output and Terminal's sends didn't arrive exactly as printed");
   }
   if (sendsInMainLine != 0)
      fail("one of Terminal's sends landed inside a main-loop line");
   if (sendsInIsrLine != 0 && !allowLongLines)
      fail("one of Terminal's sends landed inside an interrupt's line");
   if (!allowLongLines && isrLinesTried - isrLinesGot != droppedTotal)
      fail("interrupt lines lost differ from the count");
   if (reported + droppedPending != droppedTotal)
      fail("the \"lines dropped\" lines don't add up to the count");
   if (serialRing_droppedLines() != droppedTotal)
      fail("serialRing_droppedLines() differs from the count");
   if (preemptions != 0)
      fail("the scheduler's interrupt could write while the main loop was putting bytes into the ring");

   printf("seed %u, %lu steps%s: %zu bytes out (%zu trips round the ring); main loop %zu bytes intact, with %lu json "
          "replies and %lu Terminal sends; interrupt lines %lu/%lu, %u dropped\n", seed, ops,
          allowLongLines ? " with long lines" : "", wire.size(), ringOut.size() / LOG_RING, mainOrder.size(),
          jsonReplies, termSends, isrLinesGot, isrLinesTried, (unsigned)droppedTotal);
   printf("  coverage: %lu DMA starts, %lu interrupts, %lu held off while masked, %lu writes ending at the ring's "
          "end, %lu steps Terminal waited, %lu steps the main loop waited for room; %lu of Terminal's sends between "
          "two pieces of a long interrupt line\n", dmaStarts, isrRuns, isrHeldOff, writesEndingAtWrap, termWaitSteps,
          mainWaits, sendsInIsrLine);
   return failures == 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
   unsigned seed = argc > 1 ? (unsigned)strtoul(argv[1], 0, 10) : 1;
   unsigned long ops = argc > 2 ? strtoul(argv[2], 0, 10) : 3000000;
   allowLongLines = argc > 3 && argv[3][0] == 'l';
   RingTerminal t(0, 0);
   term = &t;
   int rc = run(seed, ops);
   if (rc == 0)
      printf("OK\n");
   else
      printf("FAILED\n");
   return rc;
}
