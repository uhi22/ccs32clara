

#include "EXITypes.h"

#include "appHandEXIDatatypes.h"
#include "appHandEXIDatatypesEncoder.h"
#include "appHandEXIDatatypesDecoder.h"

#include "dinEXIDatatypes.h"
#include "dinEXIDatatypesEncoder.h"
#include "dinEXIDatatypesDecoder.h"

/* ISO 15118-2:2013 ("iso1"), unless built with "make ISO1=0" (see doc/iso15118-2.md). */
#ifdef USE_ISO1
  #include "iso1EXIDatatypes.h"
  #include "iso1EXIDatatypesEncoder.h"
  #include "iso1EXIDatatypesDecoder.h"
#endif

#define EXI_TRANSMIT_BUFFER_SIZE 256

extern struct appHandEXIDocument aphsDoc; /* The application handshake document. */

#if defined(USE_ISO1) || defined(USE_SAME_DOC_FOR_ENCODER_AND_DECODER)
  /* DIN and ISO are never used at the same time, so their documents share memory. */
  union ExiDocUnion_type {
      struct dinEXIDocument dinD; /* The DIN document. */
  #ifdef USE_ISO1
      struct iso1EXIDocument iso1D; /* The ISO 15118-2:2013 document. */
  #endif
  };
  #ifdef USE_SAME_DOC_FOR_ENCODER_AND_DECODER
    /* One document for the encoder and the decoder ("make SHARED_EXI_DOC=1"). Saves half of the RAM, but a decoded
       response is overwritten as soon as the next request is prepared: see doc/iso15118-2.md before using it. */
    extern union ExiDocUnion_type ExiDoc;
    #define ExiDocEnc ExiDoc
    #define ExiDocDec ExiDoc
  #else
    extern union ExiDocUnion_type ExiDocEnc; /* The EXI document. For the encoder. */
    extern union ExiDocUnion_type ExiDocDec; /* The EXI document. For the decoder. */
  #endif
  #define dinDocEnc (ExiDocEnc.dinD)
  #define dinDocDec (ExiDocDec.dinD)
#else
  extern struct dinEXIDocument dinDocEnc; /* The DIN document. For encoder. */
  extern struct dinEXIDocument dinDocDec; /* The DIN document. For decoder. */
#endif
extern bitstream_t global_streamEnc; /* The byte stream descriptor. */
extern bitstream_t global_streamDec; /* The byte stream descriptor. */
extern uint32_t global_streamEncPos; /* The position in the stream. */
extern uint32_t global_streamDecPos; /* The position in the stream. */
extern int g_errn;

#define SESSIONID_LEN 8
extern uint8_t sessionId[SESSIONID_LEN];
extern uint8_t sessionIdLen;



/* Decoder functions *****************************************************************************************/

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_appHandExiDocument(void);
  /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
void projectExiConnector_encode_appHandExiDocument(void);
  /* precondition: aphsDoc holds a supportedAppProtocolReq. Output: global_stream.data and global_stream.pos. */
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_DinExiDocument(void);
  /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
#if defined(__cplusplus)
}
#endif

#ifdef USE_ISO1
#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_Iso1ExiDocument(void);
  /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
void projectExiConnector_prepare_Iso1ExiDocument(void);
  /* before filling and encoding the ISO document, we initialize here all its content. */
void projectExiConnector_encode_Iso1ExiDocument(void);
  /* precondition: ExiDocEnc.iso1D is filled. Output: global_stream.data and global_stream.pos. */
#if defined(__cplusplus)
}
#endif
#endif


/* Encoder functions ****************************************************************************************/
#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_prepare_DinExiDocument(void);
/* before filling and encoding the dinDocEnc, we initialize here all its content. */
#if defined(__cplusplus)
}
#endif


#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_encode_DinExiDocument(void);
  /* precondition: dinDocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
#if defined(__cplusplus)
}
#endif

/* Test functions, just for experimentation *****************************************************************/
#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_testEncode(void);
#if defined(__cplusplus)
}
#endif


#if defined(__cplusplus)
extern "C"
{
#endif
int projectExiConnector_test(int a);
#if defined(__cplusplus)
}
#endif
