
/* Interface header for myHelpers.c */

/* Global Defines */

#define STR_TMP_SIZE 400
#define MY_SERIAL_PRINTBUFFERLEN 400


/* Global Functions */

extern void addToTrace(const char * s);
extern void showAsHex(uint8_t *data, uint16_t len, const char *description);
extern void sanityCheck(const char *hint);
extern void mySerialPrint(void);




