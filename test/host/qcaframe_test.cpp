/* Host test of ccs/qcaframe.cpp: Ethernet frames from the QCA7000's read buffer, read in pieces.
   Build and run on a PC: make -C test/host */

#include "qcaframe.h"

#include <stdio.h>
#include <string.h>
#include <vector>

static int failures;

#define CHECK(cond)                                                                         \
   do                                                                                       \
   {                                                                                        \
      if (!(cond))                                                                          \
      {                                                                                     \
         printf("FAIL line %d: %s\n", __LINE__, #cond);                                     \
         failures++;                                                                        \
      }                                                                                     \
   } while (0)

typedef std::vector<uint8_t> Bytes;

/* An Ethernet frame of len bytes: first, first+1, ... */
static Bytes ethFrame(uint16_t len, uint8_t first)
{
   Bytes f;
   for (uint16_t i = 0; i < len; i++)
      f.push_back((uint8_t)(first + i));
   return f;
}

/* The frame as the QCA7000 delivers it */
static Bytes qcaFrame(const Bytes& eth)
{
   uint16_t l2 = (uint16_t)eth.size();
   uint16_t l1 = (uint16_t)(l2 + 10);
   Bytes q = { 0, 0, (uint8_t)(l1 >> 8), (uint8_t)l1, 0xAA, 0xAA, 0xAA, 0xAA, (uint8_t)l2, (uint8_t)(l2 >> 8), 0, 0 };
   q.insert(q.end(), eth.begin(), eth.end());
   q.push_back(0x55);
   q.push_back(0x55);
   return q;
}

/* Feeds the stream in reads of the given sizes (the last one repeated), and returns the frames received. */
static std::vector<Bytes> feed(const Bytes& stream, const std::vector<uint16_t>& reads, uint16_t bufSize,
                               struct QcaFrameRx* rx = 0)
{
   static uint8_t buf[1600];
   struct QcaFrameRx own;
   std::vector<Bytes> frames;
   size_t pos = 0, r = 0;
   if (!rx)
   {
      rx = &own;
      qcaFrameRx_init(rx, buf, bufSize);
   }
   while (pos < stream.size())
   {
      uint16_t n = reads[r < reads.size() ? r : reads.size() - 1];
      r++;
      if (n > stream.size() - pos)
         n = (uint16_t)(stream.size() - pos);
      /* one read: the caller loops until all of it is used */
      uint16_t used = 0;
      while (used < n)
      {
         uint16_t frameLen = 0;
         uint16_t k = qcaFrameRx_take(rx, &stream[pos + used], (uint16_t)(n - used), &frameLen);
         CHECK(k > 0);
         used = (uint16_t)(used + k);
         if (frameLen)
            frames.push_back(Bytes(rx->buf, rx->buf + frameLen));
      }
      pos += n;
   }
   return frames;
}

/* An ACK and a 1,000-byte segment queued together, read with the 1,098-byte limit: the second frame is cut by the
   first read and completed by the second. Then every other split point. */
static void testSplitAcrossReads(void)
{
   Bytes ack = ethFrame(86, 1), seg = ethFrame(1054, 50), small = ethFrame(60, 9);
   Bytes stream = qcaFrame(ack);
   Bytes q2 = qcaFrame(seg), q3 = qcaFrame(small);
   stream.insert(stream.end(), q2.begin(), q2.end());
   stream.insert(stream.end(), q3.begin(), q3.end());

   std::vector<Bytes> got = feed(stream, { 1098 }, 1086);
   CHECK(got.size() == 3);
   CHECK(got.size() == 3 && got[0] == ack && got[1] == seg && got[2] == small);

   for (uint16_t first = 1; first < stream.size(); first++)
   {
      got = feed(stream, { first, 1098 }, 1086);
      CHECK(got.size() == 3 && got[0] == ack && got[1] == seg && got[2] == small);
   }
   for (uint16_t chunk = 1; chunk <= 40; chunk++)
   {
      got = feed(stream, { chunk }, 1086);
      CHECK(got.size() == 3 && got[0] == ack && got[1] == seg && got[2] == small);
   }
}

/* A frame larger than the buffer is dropped, without writing past the buffer; the next one is fine. */
static void testTooLarge(void)
{
   Bytes big = ethFrame(200, 3), next = ethFrame(64, 7);
   Bytes stream = qcaFrame(big), q2 = qcaFrame(next);
   stream.insert(stream.end(), q2.begin(), q2.end());
   static uint8_t guarded[150];
   struct QcaFrameRx rx;
   memset(guarded, 0xEE, sizeof(guarded));
   qcaFrameRx_init(&rx, guarded, 100);
   std::vector<Bytes> got = feed(stream, { 70 }, 100, &rx);
   CHECK(got.size() == 1 && got[0] == next);
   CHECK(rx.nDropped == 1);
   for (size_t i = 100; i < sizeof(guarded); i++)
      CHECK(guarded[i] == 0xEE);
}

/* Junk before a frame (or a read that starts in the middle of one) is skipped until a valid header. */
static void testResync(void)
{
   Bytes f1 = ethFrame(80, 1), f2 = ethFrame(90, 2);
   Bytes q1 = qcaFrame(f1), q2 = qcaFrame(f2);
   Bytes stream(q1.begin() + 30, q1.end()); /* the tail of a frame whose start we never saw */
   stream.push_back(0xAA);
   stream.push_back(0x00);
   stream.insert(stream.end(), q2.begin(), q2.end());
   struct QcaFrameRx rx;
   static uint8_t buf[1086];
   qcaFrameRx_init(&rx, buf, sizeof(buf));
   std::vector<Bytes> got = feed(stream, { 1098 }, sizeof(buf), &rx);
   CHECK(got.size() == 1 && got[0] == f2);
   CHECK(rx.nSkipped > 0);
}

/* A missing end of frame drops the frame */
static void testBadFooter(void)
{
   Bytes f1 = ethFrame(80, 1), f2 = ethFrame(90, 2);
   Bytes q1 = qcaFrame(f1), q2 = qcaFrame(f2);
   q1[q1.size() - 1] = 0x00;
   Bytes stream = q1;
   stream.insert(stream.end(), q2.begin(), q2.end());
   std::vector<Bytes> got = feed(stream, { 1098 }, 1086);
   CHECK(got.size() == 1 && got[0] == f2);
}

/* A frame shorter than an Ethernet header is dropped */
static void testTooSmall(void)
{
   Bytes f1 = ethFrame(10, 1), f2 = ethFrame(60, 2);
   Bytes stream = qcaFrame(f1), q2 = qcaFrame(f2);
   stream.insert(stream.end(), q2.begin(), q2.end());
   std::vector<Bytes> got = feed(stream, { 1098 }, 1086);
   CHECK(got.size() == 1 && got[0] == f2);
}

int main(void)
{
   testSplitAcrossReads();
   testTooLarge();
   testResync();
   testBadFooter();
   testTooSmall();
   if (failures)
      printf("FAILED (%d)\n", failures);
   else
      printf("OK\n");
   return failures ? 1 : 0;
}
