/* QCA7000 receive framing, see qcaframe.h */

#include "qcaframe.h"
#include <string.h>

void qcaFrameRx_init(struct QcaFrameRx* rx, uint8_t* buf, uint16_t size)
{
   memset(rx, 0, sizeof(*rx));
   rx->buf = buf;
   rx->size = size;
}

static uint8_t isValidHeader(const uint8_t* h, uint16_t* frameLen)
{
   uint16_t l1 = (uint16_t)((h[2] << 8) | h[3]);
   uint16_t l2 = (uint16_t)(h[8] | (h[9] << 8));
   *frameLen = l2;
   /* the upper half of L1 is 0: a frame is much shorter than 64 kB */
   return h[0] == 0 && h[1] == 0 && h[4] == 0xAA && h[5] == 0xAA && h[6] == 0xAA && h[7] == 0xAA
          && (uint32_t)l2 + 10 == l1;
}

uint16_t qcaFrameRx_take(struct QcaFrameRx* rx, const uint8_t* data, uint16_t n, uint16_t* frameLen)
{
   uint16_t used = 0;

   *frameLen = 0;
   while (used < n)
   {
      if (rx->headerLen < QCAFRAME_HEADER_LEN)
      {
         rx->header[rx->headerLen++] = data[used++];
         if (rx->headerLen == QCAFRAME_HEADER_LEN)
         {
            if (isValidHeader(rx->header, &rx->frameLen))
            {
               rx->got = 0;
               rx->isDropped = rx->frameLen > rx->size || rx->frameLen < QCAFRAME_MIN_ETH_LEN;
            }
            else
            {
               /* not the start of a frame: try again one byte further on */
               memmove(rx->header, &rx->header[1], QCAFRAME_HEADER_LEN - 1);
               rx->headerLen = QCAFRAME_HEADER_LEN - 1;
               rx->nSkipped++;
            }
         }
      }
      else
      {
         /* the Ethernet frame, then the footer */
         uint16_t total = (uint16_t)(rx->frameLen + QCAFRAME_FOOTER_LEN);
         uint16_t k = (uint16_t)(total - rx->got);
         uint16_t i;
         if (k > n - used)
            k = (uint16_t)(n - used);
         for (i = 0; i < k; i++)
         {
            uint16_t pos = (uint16_t)(rx->got + i);
            if (pos >= rx->frameLen)
            {
               if (data[used + i] != 0x55)
                  rx->isDropped = 1; /* the end of frame is missing: we are out of step */
            }
            else if (!rx->isDropped)
            {
               rx->buf[pos] = data[used + i];
            }
         }
         rx->got = (uint16_t)(rx->got + k);
         used = (uint16_t)(used + k);
         if (rx->got == total)
         {
            rx->headerLen = 0;
            if (rx->isDropped)
            {
               rx->nDropped++;
            }
            else
            {
               *frameLen = rx->frameLen;
               break;
            }
         }
      }
   }
   return used;
}
