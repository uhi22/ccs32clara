
#include "projectExiConnector.h"
#include <string.h> /* for memcpy */


static uint8_t exiTransmitBuffer[EXI_TRANSMIT_BUFFER_SIZE];

/* DIN */
struct dinEXIDocument dinDocEnc;
struct dinEXIDocument dinDocDec;

/* ISO2 */
struct iso2EXIDocument iso2DocEnc;
struct iso2EXIDocument iso2DocDec;


struct appHandEXIDocument aphsDoc;
bitstream_t global_streamEnc;
bitstream_t global_streamDec;
uint32_t global_streamEncPos;
uint32_t global_streamDecPos;
int g_errn;
uint8_t sessionId[SESSIONID_LEN];
uint8_t sessionIdLen;

#define UNUSED(x) x=x; /* to avoid compiler warning for unused parameters */

void debugAddStringAndInt(char *s, int i) {
    UNUSED(s)
    UNUSED(i)
}

void projectExiConnector_decode_appHandExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */

   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_appHandExiDocument(&global_streamDec, &aphsDoc);
}

void projectExiConnector_decode_DinExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */

   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_dinExiDocument(&global_streamDec, &dinDocDec);
}

void projectExiConnector_decode_Iso2ExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */

   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_iso2ExiDocument(&global_streamDec, &iso2DocDec);
}

void projectExiConnector_prepare_DinExiDocument(void) {
   /* before filling and encoding the dinDocEnc, we initialize here all its content. */
   init_dinEXIDocument(&dinDocEnc);
   dinDocEnc.V2G_Message_isUsed = 1u;
   init_dinMessageHeaderType(&dinDocEnc.V2G_Message.Header);
   init_dinBodyType(&dinDocEnc.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(dinDocEnc.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   dinDocEnc.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}

void projectExiConnector_prepare_Iso2ExiDocument(void) {
   /* before filling and encoding the iso2DocEnc, we initialize here all its content. */
   init_iso2EXIDocument(&iso2DocEnc);
   iso2DocEnc.V2G_Message_isUsed = 1u;
   init_iso2MessageHeaderType(&iso2DocEnc.V2G_Message.Header);
   init_iso2BodyType(&iso2DocEnc.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(iso2DocEnc.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   iso2DocEnc.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}


void projectExiConnector_encode_DinExiDocument(void) {
   /* precondition: dinDocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_dinExiDocument(&global_streamEnc, &dinDocEnc);

}

void projectExiConnector_encode_Iso2ExiDocument(void) {
   /* precondition: iso2DocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_iso2ExiDocument(&global_streamEnc, &iso2DocEnc);
}
