

#include "ccs32_globals.h"

uint16_t checkpointNumber;

/* Helper functions */

void setCheckpoint(uint16_t newcheckpoint) {
    checkpointNumber = newcheckpoint;
    Param::SetInt(Param::checkpoint, newcheckpoint);
}

void addToTrace(enum Module module, const char * s) {
   if (Param::GetInt(Param::logging) & module)
      printf("[%u] %s\r\n", rtc_get_ms(), s);
   // canbus_addStringToTextTransmitBuffer(mySerialPrintOutputBuffer); /* print to the CAN */
}

void addToTrace(enum Module module, const char * s, uint8_t* data, uint16_t len) {
   if (Param::GetInt(Param::logging) & module) {
      printf("[%u] %s ", rtc_get_ms(), s);
      for (uint16_t i = 0; i < len; i++)
         printf("%02x", data[i]);
      printf("\r\n");
   }
}

void sanityCheck(const char*) {
    /* todo: check the canaries, config registers, maybe stack, ... */
}

