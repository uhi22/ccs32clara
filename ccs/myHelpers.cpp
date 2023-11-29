

#include "ccs32_globals.h"


/* Helper functions */

void addToTrace(const char * s) {
   //printf("[%d] %s\r\n", rtc_get_counter_val(), s);
   // canbus_addStringToTextTransmitBuffer(mySerialPrintOutputBuffer); /* print to the CAN */
}

void showAsHex(uint8_t *data, uint16_t len, const char *description) {
   int i;
   /*printf("%s (%d bytes): ", description, len);
   for (i=0; i<len; i++) {
       printf("%02x ", data[i]);
   }*/
}

void sanityCheck(const char *hint) {
    /* todo: check the canaries, config registers, maybe stack, ... */
}

