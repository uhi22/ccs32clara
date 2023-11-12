
/* Interface header for myHelpers.c */

/* Global Defines */

#define STR_TMP_SIZE 400
#define MY_SERIAL_PRINTBUFFERLEN 400

/* Global Variables */

extern char strTmp[STR_TMP_SIZE];
extern char mySerialPrintOutputBuffer[MY_SERIAL_PRINTBUFFERLEN];

/* Global Functions */

extern void addToTrace(char * s);
extern void showAsHex(uint8_t *data, uint16_t len, char *description);
extern void sanityCheck(char *hint);
extern void mySerialPrint(void);




