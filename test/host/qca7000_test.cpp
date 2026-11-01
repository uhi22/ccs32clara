/* Host test of ccs/qca7000.cpp: reading frames from the QCA7000 while the handlers send frames back.
   Build and run on a PC: make -C test/host

   ccs/qca7000.cpp is compiled as it is, with the SPI, its DMA and the QCA7000 simulated (qca7000_hosttest.h):
   - A DMA transfer starts when the SPI's transmit DMA is enabled, with the chip select low. It runs in the
     background: it moves on a step each time its count is polled and completes after a few steps. Then it sets the
     channel's transfer-complete flag, and the DMA interrupt (dma1_channel2_isr, which preempts the 30 ms task) runs
     at once if it is enabled. Disabling the channel before that aborts the transfer: only the bytes moved so far
     count, on both sides. The SPI's busy flag reads 0, as it does between two bytes when the DMA is late.
   - The QCA7000 holds a queue of received frames with their QCA7000 framing; a read of its buffer takes up to the
     SPI buffer's size, so frames are split across reads, and an aborted read takes only the bytes moved. A frame
     write is taken from the transmit buffer as it is when the transfer ends; a write cut short is not a frame.
   - Each frame received is answered at once with an 86-byte frame (as tcp.cpp sends an ACK from inside the receive
     loop).
   - The DWT cycle counter advances on each read of it, so a transfer that never completes times out.
   Checks: every frame arrives once, intact and in order; every frame sent reaches the QCA7000 intact; no SPI access
   starts while a transfer is under way or with the chip select still low from the last one. And with a transfer
   that never completes: it is stopped within the timeout, the chip select goes high, the read isn't parsed, the
   timeout is counted, and the reads after it work.

   usage: qca7000_test */

#include "../../ccs/qca7000.cpp"

#include <deque>
#include <stdlib.h>
#include <vector>

typedef std::vector<uint8_t> Bytes;

static int failures;

#define CHECK(cond)                                                                         \
   do                                                                                       \
   {                                                                                        \
      if (!(cond))                                                                          \
      {                                                                                     \
         if (failures < 20)                                                                 \
            printf("FAIL line %d: %s\n", __LINE__, #cond);                                  \
         failures++;                                                                        \
      }                                                                                     \
   } while (0)

/* --- the rest of Clara --- */

uint16_t checkpointNumber;
static std::vector<Bytes> received; /* frames handed to the IPv6 handler */
static std::vector<Bytes> toSend;   /* frames the handler sent */
static std::vector<Bytes> sent;     /* frames the QCA7000 received from us */
static unsigned long writesCut;     /* frame writes cut short by an abort */

uint16_t getEtherType(uint8_t* b)
{
   return (uint16_t)((b[12] << 8) | b[13]);
}

void evaluateReceivedHomeplugPacket(void)
{
   CHECK(0); /* the test sends IPv6 frames only */
}

/* A frame came in: answer it at once, as tcp.cpp's ACK does. */
void ipv6_evaluateReceivedPacket(void)
{
   received.push_back(Bytes(myethreceivebuffer, myethreceivebuffer + myethreceivebufferLen));
   Bytes ack(86);
   for (size_t i = 0; i < ack.size(); i++)
      ack[i] = (uint8_t)(rand() & 0xFF);
   memcpy(myethtransmitbuffer, ack.data(), ack.size());
   myethtransmitbufferLen = (uint16_t)ack.size();
   toSend.push_back(ack);
   myEthTransmit();
}

void composeGetSwReq(void)
{
}

/* --- the hardware --- */

HostPin DigIo::spics;
uint32_t hostSpiDr;
uint32_t rcc_ahb_frequency = 72000000;
static uint32_t cycles;
static std::deque<uint8_t> modem; /* what the QCA7000 holds for us to read */
static bool csLow;
static int spiIndex; /* byte of the current register access */
static uint8_t spiCommand;
static unsigned long violations; /* an SPI access while a transfer runs, or with the chip select still low */

static struct
{
   bool en, minc;
   uint32_t cmar;
   uint16_t cndtr;
} ch[4];
static bool tcif2, tcie2;

static struct
{
   bool running;
   uint16_t n;      /* bytes of the transfer */
   int steps, done; /* steps to complete it, and taken so far */
   bool stuck;      /* it stops moving after stuckAfter steps, and never completes */
} xfer;
static int stickNext;  /* 1: the next read never completes, 2: the next frame write never completes */
static int stuckAfter; /* steps a stuck transfer takes before it stops moving */

bool dwt_enable_cycle_counter(void)
{
   return true;
}

uint32_t dwt_read_cycle_counter(void)
{
   cycles += 100; /* about one pass of a wait loop */
   return cycles;
}

uint32_t rtc_get_ms(void)
{
   return cycles / 72000;
}

void HostPin::Clear(void)
{
   if (csLow || xfer.running)
      violations++;
   csLow = true;
   spiIndex = 0;
}

void HostPin::Set(void)
{
   csLow = false;
}

uint32_t hostSpiSr(void)
{
   return 0; /* not busy: as between two bytes when the DMA is late */
}

uint8_t spi_xfer(uint32_t, uint8_t data)
{
   if (xfer.running || !csLow)
      violations++;
   if (spiIndex == 0)
      spiCommand = data;
   int i = spiIndex++;
   if (spiCommand == 0xC3 && i >= 2) /* RDBUF_BYTE_AVA */
   {
      uint16_t n = modem.size() > 3000 ? 3000 : (uint16_t)modem.size();
      return i == 2 ? (uint8_t)(n >> 8) : (uint8_t)n;
   }
   if (spiCommand == 0xDA && i >= 2) /* SIGNATURE */
      return i == 2 ? 0xAA : 0x55;
   return 0;
}

uint32_t hostDmaCcr(uint8_t channel)
{
   return ch[channel].en ? DMA_CCR_EN : 0;
}

void dma_channel_reset(uint32_t, uint8_t channel)
{
   ch[channel].en = false;
   ch[channel].minc = false;
}

void dma_set_read_from_memory(uint32_t, uint8_t)
{
}

void dma_enable_memory_increment_mode(uint32_t, uint8_t channel)
{
   ch[channel].minc = true;
}

void dma_disable_memory_increment_mode(uint32_t, uint8_t channel)
{
   ch[channel].minc = false;
}

void dma_set_memory_address(uint32_t, uint8_t channel, uint32_t address)
{
   if (ch[channel].en)
      violations++; /* ignored by the STM32 while the channel is on */
   else
      ch[channel].cmar = address;
}

void dma_set_number_of_data(uint32_t, uint8_t channel, uint16_t number)
{
   if (ch[channel].en)
      violations++;
   else
      ch[channel].cndtr = number;
}

bool dma_get_interrupt_flag(uint32_t, uint8_t channel, uint32_t flags)
{
   return channel == 2 && (flags & DMA_TCIF) && tcif2;
}

void dma_clear_interrupt_flags(uint32_t, uint8_t channel, uint32_t flags)
{
   if (channel == 2 && (flags & DMA_TCIF))
      tcif2 = false;
}

void dma_enable_transfer_complete_interrupt(uint32_t, uint8_t channel)
{
   if (channel == 2)
      tcie2 = true;
}

void dma_disable_transfer_complete_interrupt(uint32_t, uint8_t channel)
{
   if (channel == 2)
      tcie2 = false;
}

static uint32_t addr(const void* p)
{
   return (uint32_t)(uintptr_t)p;
}

void dma_enable_channel(uint32_t, uint8_t channel)
{
   ch[channel].en = true;
}

/* The transfer starts: the SPI asks the DMA for its first byte. */
void spi_enable_tx_dma(uint32_t)
{
   CHECK(csLow && ch[2].en && ch[3].en && !xfer.running);
   CHECK(ch[3].cmar == addr(mySpiTxBuffer) && ch[3].cndtr == ch[2].cndtr);
   xfer.running = true;
   xfer.n = ch[2].cndtr;
   xfer.steps = 1 + rand() % 5;
   xfer.done = 0;
   xfer.stuck = (stickNext == 1 && mySpiTxBuffer[0] == 0x80) || (stickNext == 2 && mySpiTxBuffer[0] == 0x00);
   if (xfer.stuck)
   {
      stickNext = 0;
      if (xfer.steps <= stuckAfter)
         xfer.steps = stuckAfter + 1;
   }
}

/* The transfer ends after moved bytes (all of them, or fewer if it was aborted): the QCA7000 takes what was
   clocked out, and what it clocked back reaches memory. */
static void endTransfer(uint16_t moved)
{
   uint16_t n = xfer.n;
   const uint8_t* tx = mySpiTxBuffer;
   Bytes back(n, 0xEE);

   xfer.running = false;
   if (tx[0] == 0x80) /* read, external: two bytes for the command, then the buffer */
   {
      for (uint16_t i = 2; i < moved && !modem.empty(); i++)
      {
         back[i] = modem.front();
         modem.pop_front();
      }
   }
   else if (tx[0] == 0x00 && n >= 12) /* write, external: a frame */
   {
      uint16_t len = (uint16_t)(tx[6] | (tx[7] << 8));
      CHECK(tx[2] == 0xAA && tx[3] == 0xAA && tx[4] == 0xAA && tx[5] == 0xAA && len + 12 == n);
      CHECK(tx[10 + len] == 0x55 && tx[11 + len] == 0x55);
      if (moved == n)
         sent.push_back(Bytes(tx + 10, tx + 10 + len));
      else
         writesCut++;
   }
   else
   {
      CHECK(0); /* an unknown command */
   }
   /* what comes back goes where the receive channel points */
   if (moved > 0)
   {
      if (ch[2].cmar == addr(mySpiRxBuffer) && ch[2].minc)
      {
         CHECK(n <= sizeof(mySpiRxBuffer));
         memcpy(mySpiRxBuffer, back.data(), moved);
      }
      else if (ch[2].cmar == addr(&spiRxDiscard) && !ch[2].minc)
      {
         spiRxDiscard = back[moved - 1];
      }
      else
      {
         CHECK(0); /* the DMA writes somewhere unexpected */
      }
   }
   ch[2].cndtr = (uint16_t)(n - moved);
   ch[3].cndtr = ch[2].cndtr;
}

static uint16_t movedSoFar(void)
{
   return (uint16_t)((uint32_t)xfer.n * (uint32_t)xfer.done / (uint32_t)xfer.steps);
}

void dma_disable_channel(uint32_t, uint8_t channel)
{
   if (channel == 2 && xfer.running)
      endTransfer(movedSoFar()); /* aborted */
   ch[channel].en = false;
}

uint16_t dma_get_number_of_data(uint32_t, uint8_t channel)
{
   if (channel == 2 && xfer.running)
   {
      if (!xfer.stuck || xfer.done < stuckAfter)
         xfer.done++;
      if (xfer.done < xfer.steps)
         return (uint16_t)(xfer.n - movedSoFar());
      endTransfer(xfer.n);
      tcif2 = true;
      if (tcie2)
         dma1_channel2_isr(); /* preempts the 30 ms task */
   }
   return ch[channel].cndtr;
}

/* --- the test --- */

static Bytes ethFrame(uint16_t len)
{
   Bytes f(len);
   for (uint16_t i = 0; i < len; i++)
      f[i] = (uint8_t)(rand() & 0xFF);
   f[12] = 0x86; /* IPv6 */
   f[13] = 0xDD;
   return f;
}

static void queueFrame(const Bytes& eth)
{
   uint16_t l2 = (uint16_t)eth.size();
   uint16_t l1 = (uint16_t)(l2 + 10);
   Bytes q = { 0, 0, (uint8_t)(l1 >> 8), (uint8_t)l1, 0xAA, 0xAA, 0xAA, 0xAA, (uint8_t)l2, (uint8_t)(l2 >> 8), 0, 0 };
   q.insert(q.end(), eth.begin(), eth.end());
   q.push_back(0x55);
   q.push_back(0x55);
   modem.insert(modem.end(), q.begin(), q.end());
}

static void startRun(const std::vector<Bytes>& frames)
{
   received.clear();
   toSend.clear();
   sent.clear();
   writesCut = 0;
   violations = 0;
   for (const Bytes& f : frames)
      queueFrame(f);
}

static void readAll(void)
{
   for (int i = 0; i < 1000 && !modem.empty(); i++)
      spiQCA7000checkForReceivedData();
   waitSpiDmaDone(); /* the last frame sent */
   CHECK(modem.empty());
   CHECK(!csLow);
   CHECK(violations == 0);
}

/* Reads until the QCA7000 has nothing more, and checks what came in and what went out. */
static void run(const std::vector<Bytes>& frames, const char* name)
{
   int failuresBefore = failures;
   uint16_t timeoutsBefore = spiDmaTimeouts;
   startRun(frames);
   readAll();
   CHECK(received == frames);
   CHECK(sent == toSend);
   CHECK(spiDmaTimeouts == timeoutsBefore); /* no transfer ran into the timeout */
   if (name || failures != failuresBefore)
      printf("%s: %zu frames in, %zu sent%s\n", name ? name : "random", received.size(), sent.size(),
             failures == failuresBefore ? "" : " - FAILED");
}

/* 1 if part is in whole, in order (each frame intact) */
static bool isSubsequence(const std::vector<Bytes>& part, const std::vector<Bytes>& whole)
{
   size_t k = 0;
   for (const Bytes& f : whole)
      if (k < part.size() && part[k] == f)
         k++;
   return k == part.size();
}

/* A read that never completes: stopped within the timeout, chip select high, not parsed, counted; then the reads
   go on. stall: steps the read takes before it stops moving (0: nothing arrives; else a part of it does, and the
   frames in that part are lost). */
static void stuckRead(int stall)
{
   int failuresBefore = failures;
   std::vector<Bytes> frames;
   for (int i = 0; i < 6; i++)
      frames.push_back(ethFrame((uint16_t)(60 + rand() % 400)));
   startRun(frames);
   uint16_t timeoutsBefore = spiDmaTimeouts;
   stickNext = 1;
   stuckAfter = stall;
   uint32_t start = cycles;
   spiQCA7000checkForReceivedData();
   CHECK(spiDmaTimeouts == timeoutsBefore + 1);
   CHECK(cycles - start <= 72u * 6000u); /* stopped within about the 5 ms timeout */
   CHECK(received.empty());             /* the read wasn't parsed */
   CHECK(!csLow && !xfer.running && !tcie2);
   readAll();
   CHECK(spiDmaTimeouts == timeoutsBefore + 1);
   if (stall == 0)
      CHECK(received == frames); /* nothing was read: nothing lost */
   else
      CHECK(isSubsequence(received, frames) && !received.empty() && received.back() == frames.back());
   CHECK(sent == toSend);
   printf("read stuck after %d steps: timeout counted, %zu of %zu frames in afterwards%s\n", stall, received.size(),
          frames.size(), failures == failuresBefore ? "" : " - FAILED");
}

/* A frame write that never completes: the next SPI access stops it, raises the chip select and counts it; the frame
   is lost, everything else goes on. */
static void stuckWrite(void)
{
   int failuresBefore = failures;
   std::vector<Bytes> frames = { ethFrame(100), ethFrame(200), ethFrame(300) };
   startRun(frames);
   uint16_t timeoutsBefore = spiDmaTimeouts;
   stickNext = 2;
   stuckAfter = 1;
   readAll();
   CHECK(spiDmaTimeouts == timeoutsBefore + 1);
   CHECK(received == frames);
   CHECK(writesCut == 1 && toSend.size() == 3 && sent.size() == 2);
   CHECK(isSubsequence(sent, toSend));
   printf("frame write stuck: timeout counted, %zu of %zu frames sent%s\n", sent.size(), toSend.size(),
          failures == failuresBefore ? "" : " - FAILED");
}

int main(void)
{
   srand(1);
   qca7000setup();

   /* A read that starts with the end of a frame split across reads, followed by another frame: completing the first
      sends a frame, and the second must still be read intact. A 60-byte and a 1,080-byte frame (74 + 1,094 bytes
      with the framing) fill the first read of 1,098 bytes up to 70 bytes before the end of the second, so the second
      read is those 70 bytes and then a 100-byte frame. */
   run({ ethFrame(60), ethFrame(1080), ethFrame(100) }, "split frame then another");

   /* Random frames, so that reads end anywhere */
   size_t total = 0;
   int runs = 0;
   for (; runs < 200 && !failures; runs++)
   {
      std::vector<Bytes> frames;
      int k = 1 + rand() % 12;
      for (int i = 0; i < k; i++)
         frames.push_back(ethFrame((uint16_t)(60 + rand() % (MY_ETH_RECEIVE_BUFFER_LEN - 59))));
      run(frames, 0);
      total += frames.size();
   }
   printf("random: %d runs, %zu frames\n", runs, total);

   /* Transfers that never complete */
   stuckRead(0);
   stuckRead(1);
   stuckWrite();
   run({ ethFrame(500), ethFrame(70) }, "normal reads afterwards");

   if (failures)
      printf("FAILED (%d)\n", failures);
   else
      printf("OK\n");
   return failures ? 1 : 0;
}
