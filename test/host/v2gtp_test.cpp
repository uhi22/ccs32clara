/* Host test of ccs/v2gtp.cpp: V2GTP message assembly from TCP segments, and the SDP response.
   Build and run on a PC: make -C test/host */

#include "v2gtp.h"

#include <stdio.h>
#include <stdlib.h>
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

/* A V2GTP message: header (version 1, payload type 0x8001, length) and payload bytes 0, 1, 2, ... */
static std::vector<uint8_t> message(uint32_t payloadLen, uint8_t first = 0)
{
   std::vector<uint8_t> m = { 0x01, 0xFE, 0x80, 0x01, (uint8_t)(payloadLen >> 24), (uint8_t)(payloadLen >> 16),
                              (uint8_t)(payloadLen >> 8), (uint8_t)payloadLen };
   for (uint32_t i = 0; i < payloadLen; i++)
      m.push_back((uint8_t)(first + i));
   return m;
}

static void testSingleSegment(void)
{
   uint8_t buf[1032];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> m = message(100);

   v2gtpRx_start(&rx, buf, sizeof(buf), 1000);
   uint8_t r = v2gtpRx_segment(&rx, 1000, m.data(), (uint16_t)m.size(), &len);
   CHECK(r == (V2GTPRX_STARTED | V2GTPRX_COMPLETE));
   CHECK((size_t)len == m.size());
   CHECK(memcmp(buf, m.data(), m.size()) == 0);
   CHECK(rx.nextSeq == 1000 + m.size());
}

/* A 1 kB message in segments of every size from 1 to 600 bytes, the header split too. */
static void testSplit(void)
{
   std::vector<uint8_t> m = message(1024, 7);
   for (uint16_t seg = 1; seg <= 600; seg++)
   {
      uint8_t buf[1032];
      struct V2gtpRx rx;
      uint16_t len = 0;
      uint8_t all = 0;
      uint32_t seq = 0xFFFFFF00u; /* also across the sequence number wrap */
      v2gtpRx_start(&rx, buf, sizeof(buf), seq);
      for (size_t pos = 0; pos < m.size(); pos += seg)
      {
         uint16_t n = (uint16_t)(m.size() - pos < seg ? m.size() - pos : seg);
         uint8_t r = v2gtpRx_segment(&rx, seq + (uint32_t)pos, &m[pos], n, &len);
         if (pos + n < m.size())
            CHECK(!(r & V2GTPRX_COMPLETE));
         all |= r;
      }
      CHECK(all == (V2GTPRX_STARTED | V2GTPRX_COMPLETE));
      CHECK((size_t)len == m.size());
      CHECK(memcmp(buf, m.data(), m.size()) == 0);
      CHECK(rx.nextSeq == (uint32_t)(seq + m.size()));
   }
}

/* Retransmissions, overlaps and gaps */
static void testOrder(void)
{
   uint8_t buf[1032];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> m = message(300);

   v2gtpRx_start(&rx, buf, sizeof(buf), 5000);
   CHECK(v2gtpRx_segment(&rx, 5000, &m[0], 100, &len) == V2GTPRX_STARTED);
   CHECK(v2gtpRx_segment(&rx, 5000, &m[0], 100, &len) == V2GTPRX_DUPLICATE); /* the same again */
   CHECK(v2gtpRx_segment(&rx, 5200, &m[200], 108, &len) == V2GTPRX_GAP);     /* 100..199 missing */
   CHECK(rx.nextSeq == 5100);
   CHECK(v2gtpRx_segment(&rx, 5050, &m[50], 150, &len) == 0); /* overlaps: only 100..199 are new */
   CHECK(rx.nextSeq == 5200);
   CHECK(v2gtpRx_segment(&rx, 5200, &m[200], 108, &len) == V2GTPRX_COMPLETE);
   CHECK((size_t)len == m.size());
   CHECK(memcmp(buf, m.data(), m.size()) == 0);
}

/* A message larger than the buffer stops the receive path: nothing more is taken or acknowledged until it starts
   again (the next connection). */
static void testTooLarge(void)
{
   uint8_t buf[200];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> big = message(500);
   std::vector<uint8_t> next = message(50, 9);

   v2gtpRx_start(&rx, buf, sizeof(buf), 1);
   CHECK(v2gtpRx_segment(&rx, 1, &big[0], 128, &len) == (V2GTPRX_STARTED | V2GTPRX_ERROR));
   CHECK(rx.nextSeq == 1); /* the header is not acknowledged */
   CHECK(rx.isFailed);
   CHECK(v2gtpRx_segment(&rx, 1, &big[0], 128, &len) == V2GTPRX_ERROR);
   CHECK(v2gtpRx_segment(&rx, 1, next.data(), (uint16_t)next.size(), &len) == V2GTPRX_ERROR);
   CHECK(rx.nextSeq == 1);
   CHECK(len == 0);

   /* the largest message that fits is fine */
   std::vector<uint8_t> fits = message(sizeof(buf) - V2GTP_HEADER_LEN);
   v2gtpRx_start(&rx, buf, sizeof(buf), 1);
   CHECK(v2gtpRx_segment(&rx, 1, fits.data(), (uint16_t)fits.size(), &len) == (V2GTPRX_STARTED | V2GTPRX_COMPLETE));
   CHECK((size_t)len == fits.size());
}

/* A nonsense length (which would wrap when the header is added) is an error too, also with the header split. */
static void testNonsenseLength(void)
{
   uint8_t buf[1032];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> m = message(0);
   m[4] = 0xFF;
   m[5] = 0xFF;
   m[6] = 0xFF;
   m[7] = 0xFC;

   v2gtpRx_start(&rx, buf, sizeof(buf), 100);
   CHECK(v2gtpRx_segment(&rx, 100, &m[0], 5, &len) == V2GTPRX_STARTED);
   CHECK(v2gtpRx_segment(&rx, 105, &m[5], 3, &len) == V2GTPRX_ERROR);
   CHECK(rx.nextSeq == 105);
   CHECK(len == 0);
}

/* Data after the end of a message in the same segment (here the next message) is not taken and not acknowledged.
   When the sender repeats it, it is taken as the next message. */
static void testExtra(void)
{
   uint8_t buf[1032];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> a = message(20, 1);
   std::vector<uint8_t> b = message(30, 2);
   std::vector<uint8_t> two = a;
   two.insert(two.end(), b.begin(), b.end());

   v2gtpRx_start(&rx, buf, sizeof(buf), 0);
   CHECK(v2gtpRx_segment(&rx, 0, two.data(), (uint16_t)two.size(), &len) ==
         (V2GTPRX_STARTED | V2GTPRX_COMPLETE | V2GTPRX_EXTRA));
   CHECK((size_t)len == a.size());
   CHECK(memcmp(buf, a.data(), a.size()) == 0);
   CHECK(rx.nextSeq == a.size()); /* only message a is acknowledged */

   /* the sender repeats from the ACK number: b on its own, or the whole segment again */
   len = 0;
   CHECK(v2gtpRx_segment(&rx, 0, two.data(), (uint16_t)two.size(), &len) == (V2GTPRX_STARTED | V2GTPRX_COMPLETE));
   CHECK((size_t)len == b.size());
   CHECK(memcmp(buf, b.data(), b.size()) == 0);
   CHECK(rx.nextSeq == two.size());
}

/* A message with payload length 0 holds nothing to decode. It must not be reported as a complete message: the
   application takes only a message with a payload, so it would stay in the receive buffer and block everything after
   it (no further segment is taken while a message waits). It is acknowledged and dropped, and the stream goes on. */
static void testEmpty(void)
{
   uint8_t buf[1032];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> empty = message(0);
   std::vector<uint8_t> next = message(40, 3);

   /* alone in a segment */
   v2gtpRx_start(&rx, buf, sizeof(buf), 700);
   CHECK(v2gtpRx_segment(&rx, 700, empty.data(), (uint16_t)empty.size(), &len) == (V2GTPRX_STARTED | V2GTPRX_EMPTY));
   CHECK(len == 0);
   CHECK(rx.nextSeq == 708); /* acknowledged */
   CHECK(v2gtpRx_segment(&rx, 708, next.data(), (uint16_t)next.size(), &len) == (V2GTPRX_STARTED | V2GTPRX_COMPLETE));
   CHECK((size_t)len == next.size());
   CHECK(memcmp(buf, next.data(), next.size()) == 0);

   /* followed by the next message in the same segment: that one is taken too */
   std::vector<uint8_t> both = empty;
   both.insert(both.end(), next.begin(), next.end());
   len = 0;
   v2gtpRx_start(&rx, buf, sizeof(buf), 0);
   CHECK(v2gtpRx_segment(&rx, 0, both.data(), (uint16_t)both.size(), &len) ==
         (V2GTPRX_STARTED | V2GTPRX_EMPTY | V2GTPRX_COMPLETE));
   CHECK((size_t)len == next.size());
   CHECK(memcmp(buf, next.data(), next.size()) == 0);
   CHECK(rx.nextSeq == both.size());

   /* its header split over two segments, and sent again */
   len = 0;
   v2gtpRx_start(&rx, buf, sizeof(buf), 50);
   CHECK(v2gtpRx_segment(&rx, 50, &empty[0], 3, &len) == V2GTPRX_STARTED);
   CHECK(v2gtpRx_segment(&rx, 53, &empty[3], 5, &len) == V2GTPRX_EMPTY);
   CHECK(v2gtpRx_segment(&rx, 50, empty.data(), (uint16_t)empty.size(), &len) == V2GTPRX_DUPLICATE);
   CHECK(len == 0);
   CHECK(rx.nextSeq == 58);
}

/* A stream of messages of random sizes, some empty, cut into segments of random sizes: every message with a payload
   arrives once, in order, and nothing waits for ever. */
static void testStream(void)
{
   srand(3);
   for (int run = 0; run < 200; run++)
   {
      std::vector<std::vector<uint8_t>> msgs;
      std::vector<uint8_t> stream;
      for (int i = 0; i < 30; i++)
      {
         uint32_t payloadLen = rand() % 4 == 0 ? 0 : (uint32_t)(1 + rand() % 300);
         msgs.push_back(message(payloadLen, (uint8_t)i));
         stream.insert(stream.end(), msgs.back().begin(), msgs.back().end());
      }
      uint8_t buf[1032];
      struct V2gtpRx rx;
      uint32_t seq0 = (uint32_t)rand() * 7919u;
      v2gtpRx_start(&rx, buf, sizeof(buf), seq0);
      size_t next = 0; /* the next message with a payload expected */
      int steps = 0;
      while (rx.nextSeq - seq0 < stream.size() && steps++ < 100000)
      {
         /* the sender sends from the ACK number, a random amount */
         size_t pos = rx.nextSeq - seq0;
         uint16_t n = (uint16_t)(1 + rand() % 500);
         if (pos + n > stream.size())
            n = (uint16_t)(stream.size() - pos);
         uint16_t len = 0;
         uint8_t r = v2gtpRx_segment(&rx, seq0 + (uint32_t)pos, &stream[pos], n, &len);
         CHECK(!(r & (V2GTPRX_ERROR | V2GTPRX_GAP | V2GTPRX_DUPLICATE)));
         if (r & V2GTPRX_COMPLETE)
         {
            CHECK(len > V2GTP_HEADER_LEN); /* what the application takes */
            while (next < msgs.size() && msgs[next].size() == V2GTP_HEADER_LEN)
               next++;
            CHECK(next < msgs.size() && (size_t)len == msgs[next].size() && memcmp(buf, msgs[next].data(), len) == 0);
            next++;
         }
      }
      while (next < msgs.size() && msgs[next].size() == V2GTP_HEADER_LEN)
         next++;
      CHECK(next == msgs.size());
      CHECK(rx.nextSeq - seq0 == stream.size());
   }
}

/* Not a V2GTP header: an error, nothing acknowledged */
static void testBadHeader(void)
{
   uint8_t buf[64];
   struct V2gtpRx rx;
   uint16_t len = 0;
   std::vector<uint8_t> junk(100, 0x55);

   v2gtpRx_start(&rx, buf, sizeof(buf), 0);
   CHECK(v2gtpRx_segment(&rx, 0, junk.data(), (uint16_t)junk.size(), &len) == (V2GTPRX_STARTED | V2GTPRX_ERROR));
   CHECK(len == 0);
   CHECK(rx.nextSeq == 0);
}

static void testSdp(void)
{
   struct V2gtpSdpResponse r;
   uint8_t p[28] = { 0x01, 0xFE, 0x90, 0x01, 0, 0, 0, 20 };
   for (int i = 0; i < 16; i++)
      p[8 + i] = (uint8_t)(0xA0 + i);
   p[24] = 0xD1; /* port 0xD1C2 */
   p[25] = 0xC2;
   p[26] = 0x10; /* no TLS */
   p[27] = 0x00; /* TCP */
   CHECK(v2gtp_parseSdpResponse(p, sizeof(p), &r) == 1);
   CHECK(r.seccPort == 0xD1C2);
   CHECK(r.seccIp[0] == 0xA0 && r.seccIp[15] == 0xAF);
   CHECK(r.security == V2GTP_SDP_SECURITY_NONE);
   CHECK(r.transport == V2GTP_SDP_TRANSPORT_TCP);

   p[26] = 0x00; /* TLS only */
   CHECK(v2gtp_parseSdpResponse(p, sizeof(p), &r) == 1);
   CHECK(r.security == V2GTP_SDP_SECURITY_TLS);

   CHECK(v2gtp_parseSdpResponse(p, 27, &r) == 0); /* too short */
   p[7] = 19;
   CHECK(v2gtp_parseSdpResponse(p, sizeof(p), &r) == 0); /* wrong payload length */
   p[7] = 20;
   p[3] = 0x00;
   CHECK(v2gtp_parseSdpResponse(p, sizeof(p), &r) == 0); /* an SDP request, not a response */
   p[3] = 0x01;
   p[1] = 0xFF;
   CHECK(v2gtp_parseSdpResponse(p, sizeof(p), &r) == 0); /* not V2GTP */
}

int main(void)
{
   testSingleSegment();
   testSplit();
   testOrder();
   testTooLarge();
   testNonsenseLength();
   testExtra();
   testBadHeader();
   testEmpty();
   testStream();
   testSdp();
   if (failures)
      printf("FAILED (%d)\n", failures);
   else
      printf("OK\n");
   return failures ? 1 : 0;
}
