
/* Interface header for myHelpers.c */

/* Global Defines */

#define STR_TMP_SIZE 400
#define MY_SERIAL_PRINTBUFFERLEN 400

enum Module
{
   MOD_CONNMGR = 1,
   MOD_HWIF = 2,
   MOD_HOMEPLUG = 4,
   MOD_PEV = 8,
   MOD_QCA = 16,
   MOD_TCP = 32,
   MOD_TCPTRAFFIC = 64,
   MOD_IPV6 = 128,
   MOD_MODEMFINDER = 256
};

extern uint16_t checkpointNumber;

/* Global Functions */

extern void addToTrace(enum Module module, const char * s);
extern void showAsHex(uint8_t *data, uint16_t len, const char *description);
extern void sanityCheck(const char *hint);
extern void mySerialPrint(void);
extern void setCheckpoint(uint16_t newcheckpoint);
