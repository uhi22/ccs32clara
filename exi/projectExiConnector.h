

#include "EXITypes.h"

#include "appHandEXIDatatypes.h"
#include "appHandEXIDatatypesEncoder.h"
#include "appHandEXIDatatypesDecoder.h"

/* DIN */
#include "dinEXIDatatypes.h"
#include "dinEXIDatatypesEncoder.h"
#include "dinEXIDatatypesDecoder.h"

/* ISO1 */
#ifdef USE_ISO1
  #include "iso1EXIDatatypes.h"
  #include "iso1EXIDatatypesEncoder.h"
  #include "iso1EXIDatatypesDecoder.h"
#endif

/* ISO2 */
#ifdef USE_ISO2
  #include "iso2EXIDatatypes.h"
  #include "iso2EXIDatatypesEncoder.h"
  #include "iso2EXIDatatypesDecoder.h"
#endif

#define EXI_TRANSMIT_BUFFER_SIZE 256

union ExiDocUnion_type {
    /* to save memory, we use the same memory for DIN and ISO, because
       they never will be used at the same time. */
    struct dinEXIDocument dinD; /* The DIN document. */
    #ifdef USE_ISO1
      struct iso1EXIDocument iso1D; /* The ISO1 document. */
    #endif
    #ifdef USE_ISO2
      struct iso2EXIDocument iso2D; /* The ISO2 document. */
    #endif
};

extern struct appHandEXIDocument aphsDoc; /* The application handshake document. */

#ifdef USE_SAME_DOC_FOR_ENCODER_AND_DECODER
  /* To save half of the memory, we share the exi structure between decoder and encoder.
     This may lead to data confusion, if both would be used at the same time. */
  extern union ExiDocUnion_type ExiDoc; /* The EXI document. For encoder and decoder. */
  #define ExiDocEnc ExiDoc
  #define ExiDocDec ExiDoc
#else
  /* separate documents for the decoder and encoder help to keep consistency, but consume
     double RAM. */
  extern union ExiDocUnion_type ExiDocDec; /* The EXI document. For decoder. */
  extern union ExiDocUnion_type ExiDocEnc; /* The EXI document. For encoder. */
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

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_Iso1ExiDocument(void);
  /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_decode_Iso2ExiDocument(void);
  /* precondition: The global_streamDec.size and global_streamDec.data have been set to the byte array with EXI data. */
#if defined(__cplusplus)
}
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
void projectExiConnector_prepare_Iso1ExiDocument(void);
/* before filling and encoding the dinDocEnc, we initialize here all its content. */
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_prepare_Iso2ExiDocument(void);
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

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_encode_Iso1ExiDocument(void);
  /* precondition: dinDocEnc structure is filled. Output: global_stream.data and global_stream.pos. */
#if defined(__cplusplus)
}
#endif

#if defined(__cplusplus)
extern "C"
{
#endif
void projectExiConnector_encode_Iso2ExiDocument(void);
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
