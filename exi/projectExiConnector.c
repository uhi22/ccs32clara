
#include "projectExiConnector.h"
#include <string.h> /* for memcpy */


static uint8_t exiTransmitBuffer[EXI_TRANSMIT_BUFFER_SIZE];
#if defined(USE_ISO1) || defined(USE_SAME_DOC_FOR_ENCODER_AND_DECODER)
  #ifdef USE_SAME_DOC_FOR_ENCODER_AND_DECODER
    union ExiDocUnion_type ExiDoc; /* The EXI document. For encoder and decoder. */
  #else
    union ExiDocUnion_type ExiDocEnc; /* The EXI document. For encoder. */
    union ExiDocUnion_type ExiDocDec; /* The EXI document. For decoder. */
  #endif
#else
struct dinEXIDocument dinDocEnc;
struct dinEXIDocument dinDocDec;
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
   g_errn = decode_dinExiDocument(&global_streamDec, &dinDocDec);
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

void projectExiConnector_encode_DinExiDocument(void) {
   /* precondition: dinDocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_dinExiDocument(&global_streamEnc, &dinDocEnc);

}

#ifdef USE_ISO1
void projectExiConnector_decode_Iso1ExiDocument(void) {
   /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
   global_streamDec.pos = &global_streamDecPos;
   *(global_streamDec.pos) = 0; /* the decoder shall start at the byte 0 */
   g_errn = decode_iso1ExiDocument(&global_streamDec, &ExiDocDec.iso1D);
}

void projectExiConnector_prepare_Iso1ExiDocument(void) {
   /* before filling and encoding the ISO document, we initialize here all its content. */
   init_iso1EXIDocument(&ExiDocEnc.iso1D);
   ExiDocEnc.iso1D.V2G_Message_isUsed = 1u;
   init_iso1MessageHeaderType(&ExiDocEnc.iso1D.V2G_Message.Header);
   init_iso1BodyType(&ExiDocEnc.iso1D.V2G_Message.Body);
   /* take the sessionID from the global variable: */
   memcpy(ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytes, sessionId, SESSIONID_LEN);
   ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytesLen = sessionIdLen;
}

void projectExiConnector_encode_Iso1ExiDocument(void) {
   /* precondition: ExiDocEnc.iso1D is filled. Output: global_stream.data and global_stream.pos. */
   global_streamEnc.size = EXI_TRANSMIT_BUFFER_SIZE;
   global_streamEnc.data = exiTransmitBuffer;
   global_streamEnc.pos = &global_streamEncPos;
   *(global_streamEnc.pos) = 0; /* start adding data at position 0 */
   g_errn = encode_iso1ExiDocument(&global_streamEnc, &ExiDocEnc.iso1D);
}

/* Nothing selects ISO 15118-2 yet, so the linker (--gc-sections) would drop the
   whole iso1 codec, and its flash and RAM cost couldn't be measured. This references the encoder and the decoder
   behind a flag that nothing sets, so they are linked but never run. The ISO state machine replaces this. */
static volatile uint8_t iso1LinkedOnly = 0;

void projectExiConnector_keepIso1Linked(void) {
   if (iso1LinkedOnly) {
      projectExiConnector_decode_Iso1ExiDocument();
      projectExiConnector_prepare_Iso1ExiDocument();
      projectExiConnector_encode_Iso1ExiDocument();
   }
}
#endif
