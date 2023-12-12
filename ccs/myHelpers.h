
/* Interface header for myHelpers.c */

#ifndef MY_HELPERS_H
#define MY_HELPERS_H

/* Global Defines */

#define STR_TMP_SIZE 400
#define MY_SERIAL_PRINTBUFFERLEN 400

extern uint16_t checkpointNumber;

/* Global Functions */

extern void addToTrace(enum Module module, const char * s);
extern void showAsHex(uint8_t *data, uint16_t len, const char *description);
extern void sanityCheck(const char *hint);
extern void mySerialPrint(void);
extern void setCheckpoint(uint16_t newcheckpoint);

#endif