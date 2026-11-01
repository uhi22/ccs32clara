/* Host test of exi/appHandRequest.c: the SupportedAppProtocolReq for each value of V2gProtocols, and how the
   charger's answer is read. Build and run on a PC: make -C test/host */

extern "C" {
#include "appHandRequest.h"
#include "appHandEXIDatatypesEncoder.h"
#include "appHandEXIDatatypesDecoder.h"
}

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(cond)                                                                         \
   do                                                                                       \
   {                                                                                        \
      if (!(cond))                                                                          \
      {                                                                                     \
         printf("FAIL line %d: %s\n", __LINE__, #cond);                                     \
         failures++;                                                                        \
      }                                                                                     \
   } while (0)

/* The request Clara has always sent (pevStateMachine.cpp): DIN 70121 only, as recorded from a Hyundai Ioniq */
static const uint8_t ioniqRequest[] = { 0x80, 0x00, 0xdb, 0xab, 0x93, 0x71, 0xd3, 0x23, 0x4b, 0x71, 0xd1, 0xb9,
                                        0x81, 0x89, 0x91, 0x89, 0xd1, 0x91, 0x81, 0x89, 0x91, 0xd2, 0x6b, 0x9b,
                                        0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00, 0x00, 0x04, 0x00, 0x40 };

static struct appHandEXIDocument doc;

static uint32_t encode(uint8_t* out, uint32_t size)
{
   bitstream_t stream;
   uint32_t pos = 0;
   memset(&stream, 0, sizeof(stream));
   stream.size = size;
   stream.data = out;
   stream.pos = &pos;
   CHECK(encode_appHandExiDocument(&stream, &doc) == 0);
   return pos;
}

static void decode(const uint8_t* in, uint32_t len)
{
   bitstream_t stream;
   uint32_t pos = 0;
   memset(&stream, 0, sizeof(stream));
   stream.size = len;
   stream.data = (uint8_t*)in;
   stream.pos = &pos;
   CHECK(decode_appHandExiDocument(&stream, &doc) == 0);
}

static int isProtocol(const struct appHandAppProtocolType* p, const char* ns, uint8_t schemaId, uint8_t priority)
{
   uint16_t n = (uint16_t)strlen(ns);
   if (p->ProtocolNamespace.charactersLen != n || p->SchemaID != schemaId || p->Priority != priority
       || p->VersionNumberMajor != 2 || p->VersionNumberMinor != 0)
      return 0;
   for (uint16_t i = 0; i < n; i++)
   {
      if (p->ProtocolNamespace.characters[i] != (exi_string_character_t)ns[i])
         return 0;
   }
   return 1;
}

static const char din[] = "urn:din:70121:2012:MsgDef";
static const char iso[] = "urn:iso:15118:2:2013:MsgDef";

static void testRequests(void)
{
   uint8_t out[256];
   uint32_t len;

   /* the Ioniq request is DIN 70121 version 2.0, schema ID 1, priority 1 */
   decode(ioniqRequest, sizeof(ioniqRequest));
   CHECK(doc.supportedAppProtocolReq_isUsed);
   CHECK(doc.supportedAppProtocolReq.AppProtocol.arrayLen == 1);
   CHECK(isProtocol(&doc.supportedAppProtocolReq.AppProtocol.array[0], din, V2G_SCHEMA_ID_DIN, 1));

   /* DIN only, built here, gives the same bytes (Clara still sends the stored ones) */
   appHandRequest_prepare(&doc, V2G_OFFER_DIN_ONLY);
   len = encode(out, sizeof(out));
   CHECK(len == sizeof(ioniqRequest));
   CHECK(memcmp(out, ioniqRequest, sizeof(ioniqRequest)) == 0);

   static const struct
   {
      uint8_t offer;
      uint16_t n;
      const char* first;
      uint8_t firstId;
      const char* second;
      uint8_t secondId;
   } cases[] = {
      { V2G_OFFER_ISO_THEN_DIN, 2, iso, V2G_SCHEMA_ID_ISO1, din, V2G_SCHEMA_ID_DIN },
      { V2G_OFFER_DIN_THEN_ISO, 2, din, V2G_SCHEMA_ID_DIN, iso, V2G_SCHEMA_ID_ISO1 },
      { V2G_OFFER_ISO_ONLY, 1, iso, V2G_SCHEMA_ID_ISO1, 0, 0 },
   };
   for (const auto& c : cases)
   {
      appHandRequest_prepare(&doc, c.offer);
      len = encode(out, sizeof(out));
      CHECK(len > 0 && len < 100); /* fits the 200-byte TCP payload with room to spare */
      decode(out, len);
      CHECK(doc.supportedAppProtocolReq.AppProtocol.arrayLen == c.n);
      CHECK(isProtocol(&doc.supportedAppProtocolReq.AppProtocol.array[0], c.first, c.firstId, 1));
      if (c.n == 2)
         CHECK(isProtocol(&doc.supportedAppProtocolReq.AppProtocol.array[1], c.second, c.secondId, 2));
   }
}

static uint8_t chosen(int ok, uint8_t schemaId, uint8_t offer)
{
   memset(&doc, 0, sizeof(doc));
   doc.supportedAppProtocolRes_isUsed = 1u;
   doc.supportedAppProtocolRes.ResponseCode = ok ? appHandresponseCodeType_OK_SuccessfulNegotiation
                                                 : appHandresponseCodeType_Failed_NoNegotiation;
   doc.supportedAppProtocolRes.SchemaID = schemaId;
   doc.supportedAppProtocolRes.SchemaID_isUsed = 1u;
   return appHandRequest_chosenProtocol(&doc, offer);
}

static void testChoice(void)
{
   CHECK(chosen(1, V2G_SCHEMA_ID_ISO1, V2G_OFFER_ISO_THEN_DIN) == V2G_PROTOCOL_ISO1);
   CHECK(chosen(1, V2G_SCHEMA_ID_DIN, V2G_OFFER_ISO_THEN_DIN) == V2G_PROTOCOL_DIN);
   CHECK(chosen(1, V2G_SCHEMA_ID_DIN, V2G_OFFER_DIN_THEN_ISO) == V2G_PROTOCOL_DIN);
   CHECK(chosen(1, V2G_SCHEMA_ID_ISO1, V2G_OFFER_ISO_ONLY) == V2G_PROTOCOL_ISO1);
   CHECK(chosen(1, V2G_SCHEMA_ID_DIN, V2G_OFFER_ISO_ONLY) == V2G_PROTOCOL_NONE); /* not offered */
   CHECK(chosen(1, V2G_SCHEMA_ID_ISO1, V2G_OFFER_DIN_ONLY) == V2G_PROTOCOL_NONE); /* not offered */
   CHECK(chosen(1, 7, V2G_OFFER_ISO_THEN_DIN) == V2G_PROTOCOL_NONE);              /* unknown schema */
   CHECK(chosen(0, V2G_SCHEMA_ID_ISO1, V2G_OFFER_ISO_THEN_DIN) == V2G_PROTOCOL_NONE); /* Failed_NoNegotiation */
   memset(&doc, 0, sizeof(doc));
   doc.supportedAppProtocolRes_isUsed = 1u; /* no SchemaID */
   CHECK(appHandRequest_chosenProtocol(&doc, V2G_OFFER_ISO_THEN_DIN) == V2G_PROTOCOL_NONE);
}

int main(void)
{
   testRequests();
   testChoice();
   if (failures)
      printf("FAILED (%d)\n", failures);
   else
      printf("OK\n");
   return failures ? 1 : 0;
}
