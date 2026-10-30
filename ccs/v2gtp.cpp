/* V2G Transfer Protocol helpers, see v2gtp.h */

#include "v2gtp.h"
#include <string.h>

static uint32_t readU32(const uint8_t* p)
{
   return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | p[3];
}

uint8_t v2gtp_parseSdpResponse(const uint8_t* p, uint16_t len, struct V2gtpSdpResponse* r)
{
   /* header: version 0x01, inverted version 0xFE, payload type 0x9001, payload length 20 */
   if (len < V2GTP_HEADER_LEN + 20 || p[0] != 0x01 || p[1] != 0xFE || p[2] != 0x90 || p[3] != 0x01
       || readU32(&p[4]) != 20)
      return 0;
   /* payload: SECC IP address (16), SECC port (2), security (1), transport protocol (1) */
   memcpy(r->seccIp, &p[8], 16);
   r->seccPort = (uint16_t)((p[24] << 8) | p[25]);
   r->security = p[26];
   r->transport = p[27];
   return 1;
}

void v2gtpRx_start(struct V2gtpRx* rx, uint8_t* buf, uint16_t size, uint32_t firstSeq)
{
   rx->buf = buf;
   rx->size = size;
   rx->nextSeq = firstSeq;
   rx->len = 0;
   rx->msgLen = 0;
   rx->isFailed = 0;
}

uint8_t v2gtpRx_segment(struct V2gtpRx* rx, uint32_t seq, const uint8_t* data, uint16_t n, uint16_t* completeLen)
{
   uint8_t result = 0;
   int32_t offset = (int32_t)(seq - rx->nextSeq); /* (sequence numbers wrap: compare by difference) */

   if (rx->isFailed)
      return V2GTPRX_ERROR;
   if (n == 0)
      return 0;
   if (offset > 0)
      return V2GTPRX_GAP; /* a segment before it is missing: the sender repeats from the ACK number */
   if ((uint32_t)(-offset) >= (uint32_t)n)
      return V2GTPRX_DUPLICATE;
   data += -offset; /* skip what was received before */
   n = (uint16_t)(n + offset);

   while (n > 0)
   {
      uint16_t k;
      if (rx->msgLen == 0)
      {
         /* the header */
         if (rx->len == 0)
            result |= V2GTPRX_STARTED;
         k = V2GTP_HEADER_LEN - rx->len < n ? (uint16_t)(V2GTP_HEADER_LEN - rx->len) : n;
         memcpy(&rx->buf[rx->len], data, k);
         rx->len += k;
         if (rx->len == V2GTP_HEADER_LEN)
         {
            uint32_t payloadLen = readU32(&rx->buf[4]);
            if (rx->buf[0] != 0x01 || rx->buf[1] != 0xFE || payloadLen > (uint32_t)(rx->size - V2GTP_HEADER_LEN))
            {
               /* Not a V2GTP header, or a message we can't hold. Skipping it by its length can't be trusted: a
                  corrupt length would make us skip, and acknowledge, whatever comes next. So stop here. The header
                  is not acknowledged. */
               rx->len = 0;
               rx->isFailed = 1;
               return result | V2GTPRX_ERROR;
            }
            if (payloadLen == 0)
            {
               /* A message with no payload: there is nothing to decode, and handed on it would sit in the receive
                  buffer, which the application only takes with a payload, and block everything after it. It is
                  complete, so it is acknowledged and dropped, and whatever follows is the next message. */
               rx->len = 0;
               rx->nextSeq += k;
               data += k;
               n -= k;
               result |= V2GTPRX_EMPTY;
               continue;
            }
            rx->msgLen = V2GTP_HEADER_LEN + payloadLen;
         }
      }
      else
      {
         /* the payload */
         k = rx->msgLen - rx->len < (uint32_t)n ? (uint16_t)(rx->msgLen - rx->len) : n;
         memcpy(&rx->buf[rx->len], data, k);
         rx->len += k;
      }
      rx->nextSeq += k;
      data += k;
      n -= k;
      if (rx->msgLen != 0 && rx->len == rx->msgLen)
      {
         *completeLen = rx->len;
         rx->len = 0;
         rx->msgLen = 0;
         result |= V2GTPRX_COMPLETE;
         break;
      }
   }
   if (n > 0)
   {
      /* More after a complete message, for example the start of the next one. Not taken and not acknowledged:
         the sender sends it again, once the message just completed has been processed. */
      result |= V2GTPRX_EXTRA;
   }
   return result;
}
