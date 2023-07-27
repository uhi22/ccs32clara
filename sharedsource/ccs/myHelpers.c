

#include "ccs32_globals.h"


char strTmp[STR_TMP_SIZE];
char mySerialPrintOutputBuffer[MY_SERIAL_PRINTBUFFERLEN] = {'\0'};

/* Helper functions */

void addToTrace(char * s) {
    sprintf(mySerialPrintOutputBuffer, "[%ld] %s\r\n", HAL_GetTick(), s);
    mySerialPrint();
}

void showAsHex(uint8_t *data, uint16_t len, char *description) {
   char strHex[5];
   int i;
   sprintf(strTmp, "%s (%d bytes): ", description, len);
   for (i=0; i<len; i++) {
       sprintf(strHex, "%02x ", data[i]);
       strcat(strTmp, strHex);
   }
   addToTrace(strTmp);
}

void sanityCheck(char *hint) {
    /* todo: check the canaries, config registers, maybe stack, ... */
}

