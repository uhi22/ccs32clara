/* The SupportedAppProtocolReq, see appHandRequest.h */

#include "appHandRequest.h"
#include <string.h>

static const char nsDin[] = "urn:din:70121:2012:MsgDef";
static const char nsIso1[] = "urn:iso:15118:2:2013:MsgDef";

static void setProtocol(struct appHandAppProtocolType* p, const char* ns, uint8_t schemaId, uint8_t priority)
{
   uint16_t i, n = (uint16_t)strlen(ns);

   init_appHandAppProtocolType(p);
   for (i = 0; i < n; i++)
      p->ProtocolNamespace.characters[i] = (exi_string_character_t)ns[i];
   p->ProtocolNamespace.charactersLen = n;
   p->VersionNumberMajor = 2;
   p->VersionNumberMinor = 0;
   p->SchemaID = schemaId;
   p->Priority = priority;
}

void appHandRequest_prepare(struct appHandEXIDocument* doc, uint8_t offer)
{
   struct appHandAppProtocolType* list;
   uint16_t n = 0;

   init_appHandEXIDocument(doc);
   doc->supportedAppProtocolReq_isUsed = 1u;
   init_appHandAnonType_supportedAppProtocolReq(&doc->supportedAppProtocolReq);
   list = doc->supportedAppProtocolReq.AppProtocol.array;
   if (offer == V2G_OFFER_ISO_THEN_DIN || offer == V2G_OFFER_ISO_ONLY)
   {
      setProtocol(&list[n], nsIso1, V2G_SCHEMA_ID_ISO1, (uint8_t)(n + 1));
      n++;
   }
   if (offer != V2G_OFFER_ISO_ONLY)
   {
      setProtocol(&list[n], nsDin, V2G_SCHEMA_ID_DIN, (uint8_t)(n + 1));
      n++;
   }
   if (offer == V2G_OFFER_DIN_THEN_ISO)
   {
      setProtocol(&list[n], nsIso1, V2G_SCHEMA_ID_ISO1, (uint8_t)(n + 1));
      n++;
   }
   doc->supportedAppProtocolReq.AppProtocol.arrayLen = n;
}

uint8_t appHandRequest_chosenProtocol(const struct appHandEXIDocument* doc, uint8_t offer)
{
   const struct appHandAnonType_supportedAppProtocolRes* res = &doc->supportedAppProtocolRes;
   uint8_t dinOffered = offer != V2G_OFFER_ISO_ONLY;
   uint8_t isoOffered = offer != V2G_OFFER_DIN_ONLY;

   if (!doc->supportedAppProtocolRes_isUsed || res->ResponseCode == appHandresponseCodeType_Failed_NoNegotiation
       || !res->SchemaID_isUsed)
      return V2G_PROTOCOL_NONE;
   if (dinOffered && res->SchemaID == V2G_SCHEMA_ID_DIN)
      return V2G_PROTOCOL_DIN;
   if (isoOffered && res->SchemaID == V2G_SCHEMA_ID_ISO1)
      return V2G_PROTOCOL_ISO1;
   return V2G_PROTOCOL_NONE;
}
