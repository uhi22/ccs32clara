
/* XCP slave
 * Universal Calibration Protocol
 * Specification: https://cdn.vector.com/cms/content/application-areas/ecu-calibration/xcp/XCP_ReferenceBook_V3.0_EN.pdf
 * Extensive example implementation: https://github.com/christoph2/cxcp
 
 We use a very simplified way here, and support only one very basic feature: SHORT_UPLOAD
   Request (master pdu): F4 <len> <dontcare> <dontcare> <mtaLL> <mtaLH> <mtaHL> <mtaHH>
   Response (slave pdu): FF <data0> <data1> <data1> <data1> 00 00 00
   We sent always 4 bytes of data, no matter what the master requested in the <len>.
*/

#include "ccs32_globals.h"
#include "main.h"

#define XCP_COMMAND_SHORT_UPLOAD 0xF4

volatile uint32_t xcpCounterRxInterrupts;
uint32_t xcpCounterRxInterruptsOld;
volatile uint8_t xcpMasterPdu[8];
uint8_t xcpSlavePdu[8];
uint8_t xcp_isTransmitMessageReady;

/* The CAN receive interrupt. This function is called when the CAN driver
 * found a message with the CAN ID of the XCP master. */
void xcp_CanRxInterrupt(void) {
    /* just copy the data, which will be later processed in task context. */
    xcpMasterPdu[0] = canRxData[0];
    xcpMasterPdu[1] = canRxData[1];
    xcpMasterPdu[2] = canRxData[2];
    xcpMasterPdu[3] = canRxData[3];
    xcpMasterPdu[4] = canRxData[4];
    xcpMasterPdu[5] = canRxData[5];
    xcpMasterPdu[6] = canRxData[6];
    xcpMasterPdu[7] = canRxData[7];
    xcpCounterRxInterrupts++; /* inform the processing task, that we have new data. */
}

/* The SHORT_UPLOAD handler.
   Extracts the address and length information from the master PDU, and
   fills the slave PDU with the memory content. */
void xcp_ShortUpload(void) {
  //uint8_t len; /* How many bytes does the master want to read? */
  uint32_t mta; /* memory adress, from where the master wants to read data */
  //len = xcpMasterPdu[1];
  /* mtaExt = xcpMasterPdu[3]; MTA_Extension is not used */
  mta = xcpMasterPdu[7];
  mta<<=8;
  mta += xcpMasterPdu[6];
  mta<<=8;
  mta += xcpMasterPdu[5];
  mta<<=8;
  mta += xcpMasterPdu[4];
  //sprintf(strTmp, "[XCP] short_upload addr %lx size %d.", mta, len); addToTrace(strTmp);
  xcpSlavePdu[0] = 0xFF; /* Positive response packet code RES = 0xFF */
  xcpSlavePdu[1] = *((uint8_t*)mta);  /* copy four bytes of data from the memory to the response message. */
  xcpSlavePdu[2] = *((uint8_t*)mta+1);
  xcpSlavePdu[3] = *((uint8_t*)mta+2);
  xcpSlavePdu[4] = *((uint8_t*)mta+3);
  xcp_isTransmitMessageReady = 1; /* this triggers the transmission */
}

/* Cyclic runnable. The more often this is called, the faster is the response. */
void xcp_Mainfunction(void) {
    if (xcpCounterRxInterrupts != xcpCounterRxInterruptsOld) {
        //sprintf(strTmp, "[XCP] interrupts: %d", xcpCounterRxInterrupts);
        //addToTrace(strTmp);
        xcpCounterRxInterruptsOld = xcpCounterRxInterrupts;
        if (xcpMasterPdu[0] == XCP_COMMAND_SHORT_UPLOAD) {
            xcp_ShortUpload();
        }
    }
}

/* Initialization function */
void xcp_Init(void) {
    xcpCounterRxInterrupts = 0;
    xcpCounterRxInterruptsOld = 0;
}
