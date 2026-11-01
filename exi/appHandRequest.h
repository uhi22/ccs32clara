/* The SupportedAppProtocolReq (application handshake): which V2G protocols the car offers, and which one the charger
   chose. Plain functions on an appHand document, without hardware or global state, so that they can be tested on a
   PC (test/host). */

#ifndef APPHANDREQUEST_H
#define APPHANDREQUEST_H

#include "appHandEXIDatatypes.h"

/* What the car offers (parameter V2gProtocols) */
#define V2G_OFFER_DIN_ONLY      0
#define V2G_OFFER_ISO_THEN_DIN  1
#define V2G_OFFER_DIN_THEN_ISO  2
#define V2G_OFFER_ISO_ONLY      3

/* The protocol of a session (value V2gProtocol) */
#define V2G_PROTOCOL_NONE  0
#define V2G_PROTOCOL_DIN   1
#define V2G_PROTOCOL_ISO1  2 /* ISO 15118-2:2013 */

/* The schema IDs the car gives the protocols in its request */
#define V2G_SCHEMA_ID_DIN   1
#define V2G_SCHEMA_ID_ISO1  2

#if defined(__cplusplus)
extern "C"
{
#endif

/* Fills doc with a SupportedAppProtocolReq for the offer: DIN 70121 ("urn:din:70121:2012:MsgDef") and/or
   ISO 15118-2:2013 ("urn:iso:15118:2:2013:MsgDef"), both version 2.0, with priority 1 for the first. */
void appHandRequest_prepare(struct appHandEXIDocument* doc, uint8_t offer);

/* The protocol the charger chose in its SupportedAppProtocolRes for our offer, or V2G_PROTOCOL_NONE if it found none
   or chose a schema we didn't offer. */
uint8_t appHandRequest_chosenProtocol(const struct appHandEXIDocument* doc, uint8_t offer);

#if defined(__cplusplus)
}
#endif

#endif
