/* Non-blocking serial output.

   libopeninv's Terminal sends a line by DMA and then waits, busily, until the line before it has gone. At 921600 baud
   (8N2, about 84 bytes/ms) a 30 ms cycle that prints a few kB of trace (the TCP connection, a modem search with
   several answers) is held up for tens of ms, and with libopeninv's scheduler an overrunning task then isn't run
   again for 655 ms.

   RingTerminal::PutChar collects each line (one buffer for the main loop, one for interrupt context) and puts it into
   a ring buffer. DMA2 channel 5 sends the ring to UART4, and its interrupt starts the next piece.
   - Interrupt context (the scheduler's tasks) never waits: a line that doesn't fit is dropped whole and counted, and
     once a second a line says how many were dropped.
   - The main loop (terminal commands, and their replies such as "json") waits for room instead, so its output
     arrives complete and unchanged, as with libopeninv's Terminal. A line longer than LINE_LEN goes into the ring in
     pieces; an interrupt's line can land between two pieces of a long main-loop line, as it could with libopeninv's
     Terminal, but never inside a piece and never inside another line.

   One DMA channel, two users. libopeninv's Terminal still sends some text itself on DMA2 channel 5: its replies to
   unknown commands, "fastuart", "echo" and "enableuart", and binary output (SendBinary, "stream"). Before each send it
   waits for the channel's transfer-complete flag (TCIF), then disables the channel, programs it, clears TCIF and
   enables it. The ring keeps to that protocol, so the two don't program the channel over each other:
   - The ring's interrupt is enabled (TCIE) only while the ring needs it: during its own transfer, or while it waits
     for one of Terminal's to finish. When the ring has nothing more to send, it disables TCIE and leaves TCIF set, so
     that Terminal's next send can go. It clears TCIF only to start a transfer of its own.
   - The ring starts a transfer only when the channel is free: enabled with nothing left to send (the last transfer,
     the ring's or Terminal's, is complete). A disabled channel, after the ring's first transfer, means Terminal is
     programming it: the ring leaves it alone, and is started again by its next write or the 100 ms task.
   - If Terminal's transfer is under way, the ring waits for its completion interrupt.
   - The interrupt checks that a completed transfer was the ring's own (its memory address) before it moves the ring
     on.
   - While the main loop is in Terminal::Run, where Terminal sends, interrupt context doesn't start or chain ring
     transfers (see Run), so it can't get between Terminal's wait and its programming of the channel, and Terminal
     isn't kept waiting by a busy ring. One exception: the DMA interrupt goes on with a line that a transfer ending at
     the ring's end has split, so that Terminal's reply can't land inside it.
   What remains: the terminal's echo of typed characters (usart_send_blocking, not DMA) still goes straight to the
   UART and can land inside a line, as before. A reply of Terminal's can land between two pieces of an interrupt's line
   longer than LINE_LEN, if the ring runs dry between them.

   Masking. The ring is shared by the main loop, the scheduler's interrupt (priority 0xE0) and this DMA interrupt
   (0xD0). Writers raise BASEPRI to 0xD0 while they copy a line into the ring (a few us), which holds off every
   interrupt of priority 0xD0 and lower: this one, the SPI DMA interrupt, the scheduler and CAN (0xF0).

   test/serialRing builds this file on a PC, with the DMA, the interrupts and Terminal's own sends simulated
   (make -C test/serialRing run). */

#ifdef SERIALRING_HOST_TEST
#include "serialRing_hosttest.h" /* test/serialRing: the DMA, the masking and the clocks, simulated */
#else
#include "ccs32_globals.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dma.h>
#endif
#include "serialRing.h"

#define LOG_RING        2048   /* bytes in the ring: 24 ms of output at 921600 baud */
#define RING_LIMIT      (LOG_RING - 3) /* every write but a line end stays below this, so a line end always fits */
#define LINE_LEN        128    /* a line longer than this is sent in pieces */
#define LOG_MASK_LEVEL  0xD0   /* BASEPRI while writing: holds off priorities 0xD0 to 0xFF */

static uint8_t ring[LOG_RING];
static volatile uint16_t head;   /* next byte written */
static volatile uint16_t tail;   /* next byte sent */
static volatile uint16_t dmaLen; /* bytes of the ring's transfer from tail; 0 when the ring has none under way */
static volatile uint8_t started; /* the ring has used the channel at least once */
static volatile uint8_t terminalBusy; /* the main loop is in Terminal::Run, where Terminal may send itself */
static uint8_t finishLine;            /* the DMA interrupt goes on with a line the ring has started sending */
static volatile uint32_t putCount;    /* bytes put into the ring since power-on (wraps) */
static volatile uint32_t sentCount;   /* ...of them, sent */
static uint32_t mainPutCount;         /* putCount after the main loop's last piece */

struct LineBuf
{
   char buf[LINE_LEN];
   uint8_t len;
   uint8_t continued; /* a piece of this line has already gone into the ring */
   uint8_t broken;    /* a piece didn't fit: the rest of the line is discarded */
};
/* mainLine: the main loop. isrLine: every interrupt context. Sharing isrLine is safe only while the scheduler's
   interrupt (TIM4) is the only interrupt that prints: another one could interrupt it halfway through a line. */
static LineBuf mainLine, isrLine;

static uint32_t droppedTotal;   /* lines dropped since power-on */
static uint16_t droppedPending; /* since the last report */
static uint8_t dropReportCount;

#ifndef SERIALRING_HOST_TEST /* (the host test has its own) */
static inline uint32_t maskLog(void)
{
   uint32_t old, level = LOG_MASK_LEVEL;
   __asm__ volatile ("mrs %0, basepri" : "=r"(old));
   __asm__ volatile ("msr basepri_max, %0" : : "r"(level) : "memory");
   return old;
}

static inline void unmaskLog(uint32_t old)
{
   __asm__ volatile ("msr basepri, %0" : : "r"(old) : "memory");
}

/* The active exception: 0 in the main loop. */
static inline uint32_t currentIpsr(void)
{
   uint32_t ipsr;
   __asm__ volatile ("mrs %0, ipsr" : "=r"(ipsr));
   return ipsr;
}

/* 1 if all interrupts are disabled (PRIMASK): then nothing can drain the ring, so the main loop mustn't wait. */
static inline uint32_t interruptsDisabled(void)
{
   uint32_t primask;
   __asm__ volatile ("mrs %0, primask" : "=r"(primask));
   return primask & 1;
}

static inline uint8_t dmaEnabled(void)
{
   return (DMA_CCR(DMA2, DMA_CHANNEL5) & DMA_CCR_EN) != 0;
}

static inline uint16_t dmaRemaining(void)
{
   return (uint16_t)DMA_CNDTR(DMA2, DMA_CHANNEL5);
}

static inline uint32_t dmaMemoryAddress(void)
{
   return DMA_CMAR(DMA2, DMA_CHANNEL5);
}
#endif

static inline uint16_t ringUsed(void)
{
   return (uint16_t)((head + LOG_RING - tail) % LOG_RING);
}

/* Starts the ring's next contiguous piece, if the ring has something to send and the channel is free (see above).
   Called masked, or from the DMA interrupt. */
static void startDma(void)
{
   uint16_t h = head, t = tail, n;

   if (dmaLen != 0 || h == t)
      return;
   if (terminalBusy && currentIpsr() != 0 && !finishLine)
      return; /* only the main loop starts the ring while Terminal may send (see Run) */
   if (dmaEnabled() ? dmaRemaining() != 0 : started)
   {
      /* Terminal's transfer is under way: its completion interrupt starts the ring. (A disabled channel: Terminal is
         programming it; the ring is started by its next write or the 100 ms task.) */
      if (dmaEnabled())
         dma_enable_transfer_complete_interrupt(DMA2, DMA_CHANNEL5);
      return;
   }
   n = h > t ? h - t : LOG_RING - t;
   dma_disable_channel(DMA2, DMA_CHANNEL5);
   dma_set_memory_address(DMA2, DMA_CHANNEL5, (uint32_t)(uintptr_t)&ring[t]);
   dma_set_number_of_data(DMA2, DMA_CHANNEL5, n);
   dma_clear_interrupt_flags(DMA2, DMA_CHANNEL5, DMA_TCIF);
   dma_enable_transfer_complete_interrupt(DMA2, DMA_CHANNEL5);
   dmaLen = n;
   started = 1;
   dma_enable_channel(DMA2, DMA_CHANNEL5);
}

extern "C" void dma2_channel4_5_isr(void)
{
   if (!dma_get_interrupt_flag(DMA2, DMA_CHANNEL5, DMA_TCIF))
      return;
   if (dmaLen != 0)
   {
      if (dmaMemoryAddress() == (uint32_t)(uintptr_t)&ring[tail])
      {
         tail = (uint16_t)((tail + dmaLen) % LOG_RING); /* the ring's transfer is complete */
         sentCount += dmaLen;
      }
      /* else Terminal's send aborted the ring's transfer: that piece goes again */
      dmaLen = 0;
   }
   /* A transfer that ended at the ring's end can stop in the middle of a line. The rest of that line goes on at once,
      even while Terminal::Run runs, so that Terminal's reply can't land inside it. That is safe here: this interrupt
      runs as soon as TCIF is set, before Terminal, which waits for TCIF in the main loop, can see it. */
   finishLine = ring[(tail + LOG_RING - 1) % LOG_RING] != '\n';
   startDma();
   finishLine = 0;
   if (dmaLen == 0)
   {
      /* Nothing of the ring's is under way: stop interrupting, and leave TCIF set for Terminal's next send. */
      dma_disable_transfer_complete_interrupt(DMA2, DMA_CHANNEL5);
   }
}

static inline void ringPut(uint8_t c)
{
   uint16_t h = head;
#ifdef SERIALRING_HOST_TEST
   hostPreemptionPoint(); /* the host test may run an interrupt here, if the masking allows it */
   h = head;
#endif
   ring[h] = c;
   putCount++;
#ifdef SERIALRING_HOST_TEST
   hostRecordByte(c); /* the host test records what goes into the ring */
#endif
   head = (uint16_t)(h + 1 == LOG_RING ? 0 : h + 1);
}

/* Copies n bytes into the ring if they fit below limit. Called masked. */
static uint8_t ringWrite(const char* data, uint16_t n, uint16_t limit)
{
   if (ringUsed() + n > limit)
      return 0;
   for (uint16_t i = 0; i < n; i++)
      ringPut((uint8_t)data[i]);
   return 1;
}

static void countDrop(void)
{
   droppedTotal++;
   if (droppedPending < 0xFFFF)
      droppedPending++;
}

/* A piece of a main-loop line: waits for room, so nothing is lost or changed. */
static void commitMainLine(LineBuf* lb)
{
   for (;;)
   {
      uint32_t old = maskLog();
      uint8_t done = ringWrite(lb->buf, lb->len, RING_LIMIT);
      if (done)
         mainPutCount = putCount;
      startDma();
      unmaskLog(old);
      if (done)
         break;
      if (interruptsDisabled())
      {
         countDrop(); /* nothing could make room: drop it rather than wait for ever */
         break;
      }
#ifdef SERIALRING_HOST_TEST
      hostWaitForRoom(); /* the DMA and the interrupts run */
#endif
   }
   lb->len = 0;
}

/* A piece of an interrupt's line: never waits. A line that doesn't fit is dropped whole. Only a line longer than
   LINE_LEN, whose first piece has already gone, can be cut: a further piece may go a line length beyond the first
   piece's limit, and if even that doesn't fit, the line is ended with "\r\n" at once (always room for that), so the
   next line still starts clean. */
static void commitIsrLine(LineBuf* lb)
{
   uint32_t old = maskLog();
   uint8_t ends = lb->len > 0 && lb->buf[lb->len - 1] == '\n';

   if (lb->broken)
   {
      /* the rest of a line that couldn't go out: discarded up to its end */
      if (ends)
         lb->broken = 0;
   }
   else if (!lb->continued)
   {
      /* the first piece leaves room for a line length more, so that a long line can go on */
      if (ringWrite(lb->buf, lb->len, RING_LIMIT - LINE_LEN))
      {
         lb->continued = !ends;
      }
      else
      {
         countDrop();
         lb->broken = !ends;
      }
   }
   else
   {
      if (ringWrite(lb->buf, lb->len, RING_LIMIT))
      {
         lb->continued = !ends;
      }
      else
      {
         countDrop();
         ringWrite("\r\n", 2, LOG_RING - 1);
         lb->continued = 0;
         lb->broken = !ends;
      }
   }
   startDma();
   lb->len = 0;
   unmaskLog(old);
}

RingTerminal::RingTerminal(uint32_t usart, const TERM_CMD* commands)
: Terminal(usart, commands)
{
   /* The Terminal has set up DMA2 channel 5 for UART4 TX; the ring uses it too (see above), with its interrupt. */
   nvic_set_priority(NVIC_DMA2_CHANNEL4_5_IRQ, LOG_MASK_LEVEL);
   nvic_enable_irq(NVIC_DMA2_CHANNEL4_5_IRQ);
   /* Terminal's first send doesn't wait for TCIF, so it would abort a ring transfer under way. Make it now, while the
      ring is empty: a line end. From then on Terminal waits for TCIF, as the ring expects. */
   SendBinary((const uint8_t*)"\r\n", 2);
}

/* Terminal sends itself only from Terminal::Run (its replies, and the commands it calls). While it runs, only the main
   loop, which is where Terminal runs, starts the ring: interrupt context neither starts a transfer nor chains the next
   one (but for the rest of a line split at the ring's end, see the DMA interrupt). So Terminal and the ring never
   program the channel over each other, and when Terminal waits for TCIF the ring lets it go after its current transfer,
   however much the interrupts print meanwhile (they fill the ring, and drop as usual when it is full). Afterwards the
   main loop starts the ring again. Terminal::Run returns at once when nothing was typed, so outside a command the ring
   is held back for microseconds. */
void RingTerminal::Run()
{
   uint32_t old;

   /* Terminal's own replies mustn't overtake what the main loop printed before them: let that go out first. */
   while ((int32_t)(sentCount - mainPutCount) < 0 && !interruptsDisabled())
   {
      old = maskLog();
      startDma();
      unmaskLog(old);
#ifdef SERIALRING_HOST_TEST
      hostWaitForRoom(); /* the DMA and the interrupts run */
#endif
   }
   terminalBusy = 1;
   Terminal::Run();
   terminalBusy = 0;
   old = maskLog();
   startDma();
   unmaskLog(old);
}

void RingTerminal::PutChar(char c)
{
   uint8_t inMain = currentIpsr() == 0;
   LineBuf* lb = inMain ? &mainLine : &isrLine;

   lb->buf[lb->len++] = c;
   if (c == '\n' || lb->len == LINE_LEN)
   {
      if (inMain)
         commitMainLine(lb);
      else
         commitIsrLine(lb);
   }
}

/* Appends text and returns the new end. */
static char* putText(char* p, const char* text)
{
   while (*text)
      *p++ = *text++;
   return p;
}

/* Appends v in decimal and returns the new end. */
static char* putUnsigned(char* p, uint32_t v)
{
   char digits[10];
   int n = 0;

   do
   {
      digits[n++] = (char)('0' + v % 10);
      v /= 10;
   } while (v != 0);
   while (n > 0)
      *p++ = digits[--n];
   return p;
}

/* Once a second, if anything was dropped: "[<ms>] Serial output: <n> lines dropped (ring full)". Formatted here rather
   than by printf, to keep printf off the scheduler's deepest call chain. If it doesn't fit either, it waits for the
   next second. Called masked. */
static void reportDrops(void)
{
   char line[64]; /* "[4294967295] Serial output: 65535 lines dropped (ring full)\r\n" is 61 */
   char* p = line;

   if (droppedPending == 0)
      return;
   *p++ = '[';
   p = putUnsigned(p, rtc_get_ms());
   p = putText(p, "] Serial output: ");
   p = putUnsigned(p, droppedPending);
   p = putText(p, " lines dropped (ring full)\r\n");
   if (ringWrite(line, (uint16_t)(p - line), RING_LIMIT))
      droppedPending = 0;
}

void serialRing_cyclic100ms(void)
{
   uint32_t old = maskLog();
   if (++dropReportCount >= 10)
   {
      dropReportCount = 0;
      reportDrops();
   }
   startDma(); /* also starts the ring if a write found the channel busy with Terminal's send */
   unmaskLog(old);
}

uint32_t serialRing_droppedLines(void)
{
   return droppedTotal;
}
