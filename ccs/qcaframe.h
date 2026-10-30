/* QCA7000 receive framing: splits the bytes read from the QCA7000's read buffer into Ethernet frames. The bytes come
   in reads of limited size, so a frame may start in one read and end in the next; this keeps the state in between.
   Plain functions without hardware or global state, so that they can be tested on a PC (test/host). */

#ifndef QCAFRAME_H
#define QCAFRAME_H

#include <stdint.h>

/* Each frame in the read buffer: 4 bytes length L1 (big endian, the rest of the frame), 4 bytes start of frame
   AA AA AA AA, 2 bytes frame length L2 (little endian), 2 bytes reserved, L2 bytes Ethernet frame, 2 bytes end of
   frame 55 55. So L1 = L2 + 10. */
#define QCAFRAME_HEADER_LEN 12
#define QCAFRAME_FOOTER_LEN 2
#define QCAFRAME_MIN_ETH_LEN 14 /* an Ethernet header */

struct QcaFrameRx
{
   uint8_t* buf;                         /* where the Ethernet frame is collected */
   uint16_t size;                        /* its size */
   uint8_t header[QCAFRAME_HEADER_LEN];  /* the header collected so far */
   uint8_t headerLen;                    /* QCAFRAME_HEADER_LEN once the header is complete and valid */
   uint16_t frameLen;                    /* L2 of the current frame */
   uint16_t got;                         /* bytes of its Ethernet frame and footer taken so far */
   uint8_t isDropped;                    /* the current frame is not passed on (too large or too small, bad footer) */
   uint16_t nSkipped;                    /* statistics: bytes skipped while looking for a frame start */
   uint16_t nDropped;                    /* statistics: frames dropped */
};

void qcaFrameRx_init(struct QcaFrameRx* rx, uint8_t* buf, uint16_t size);

/* Takes up to n bytes read from the QCA7000 and returns how many it used. It stops after the end of a complete
   frame: *frameLen is then its length, and the Ethernet frame is in buf; otherwise *frameLen is 0. Call it again
   with the rest of the data. Bytes that don't make a complete frame stay in rx for the next read. Data that doesn't
   look like a frame header is skipped byte by byte until a valid header is found. */
uint16_t qcaFrameRx_take(struct QcaFrameRx* rx, const uint8_t* data, uint16_t n, uint16_t* frameLen);

#endif
