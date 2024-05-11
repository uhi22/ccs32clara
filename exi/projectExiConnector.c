
#include "projectExiConnector.h"
#include <string.h> /* for memcpy */


static uint8_t exiTransmitBuffer[EXI_TRANSMIT_BUFFER_SIZE];

#ifdef USE_SAME_DOC_FOR_ENCODER_AND_DECODER
  union ExiDocUnion_type ExiDoc; /* The EXI document. For encoder and decoder. */
#else
  union ExiDocUnion_type ExiDocDec; /* The EXI document. For decoder. */
  union ExiDocUnion_type ExiDocEnc; /* The EXI document. For encoder. */
#endif

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
   g_errn = decode_dinExiDocument(&global_streamDec, &ExiDocDec.dinD);
}

#ifdef USE_ISO1
void projectExiConnector_decode_Iso1ExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */

   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_iso1ExiDocument(&global_streamDec, &ExiDocDec.iso1D);
}
#endif

#ifdef USE_ISO2
void projectExiConnector_decode_Iso2ExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */

   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_iso2ExiDocument(&global_streamDec, &ExiDocDec.iso2D);
}
#endif

void projectExiConnector_prepare_DinExiDocument(void) {
   /* before filling and encoding the dinDocEnc, we initialize here all its content. */
   init_dinEXIDocument(&ExiDocEnc.dinD);
   ExiDocEnc.dinD.V2G_Message_isUsed = 1u;
   init_dinMessageHeaderType(&ExiDocEnc.dinD.V2G_Message.Header);
   init_dinBodyType(&ExiDocEnc.dinD.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}

#ifdef USE_ISO1
void projectExiConnector_prepare_Iso1ExiDocument(void) {
   /* before filling and encoding the iso1DocEnc, we initialize here all its content. */
   init_iso1EXIDocument(&ExiDocEnc.iso1D);
   ExiDocEnc.iso1D.V2G_Message_isUsed = 1u;
   init_iso1MessageHeaderType(&ExiDocEnc.iso1D.V2G_Message.Header);
   init_iso1BodyType(&ExiDocEnc.iso1D.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}
#endif

#ifdef USE_ISO2
void projectExiConnector_prepare_Iso2ExiDocument(void) {
   /* before filling and encoding the iso2DocEnc, we initialize here all its content. */
   init_iso2EXIDocument(&ExiDocEnc.iso2D);
   ExiDocEnc.iso2D.V2G_Message_isUsed = 1u;
   init_iso2MessageHeaderType(&ExiDocEnc.iso2D.V2G_Message.Header);
   init_iso2BodyType(&ExiDocEnc.iso2D.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(ExiDocEnc.iso2D.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   ExiDocEnc.iso2D.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}
#endif

void projectExiConnector_encode_DinExiDocument(void) {
   /* precondition: dinDocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_dinExiDocument(&global_streamEnc, &ExiDocEnc.dinD);

}

#ifdef USE_ISO1
void projectExiConnector_encode_Iso1ExiDocument(void) {
   /* precondition: iso2DocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_iso1ExiDocument(&global_streamEnc, &ExiDocEnc.iso1D);
}
#endif

#ifdef USE_ISO2
void projectExiConnector_encode_Iso2ExiDocument(void) {
   /* precondition: iso2DocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_iso2ExiDocument(&global_streamEnc, &ExiDocEnc.iso2D);
}
#endif