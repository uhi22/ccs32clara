#include "ccs32_globals.h"
#include "projectExiConnector.h"

/* The Charging State Machine for the car */
//STATE_ENTRY(internalName, friendlyName, timeout in s)
#define STATE_LIST \
   STATE_ENTRY(NotYetInitialized, Off, 0) \
   STATE_ENTRY(Connecting, Connecting, 0) \
   STATE_ENTRY(Connected, Connected, 0) \
   STATE_ENTRY(WaitForSupportedApplicationProtocolResponse, NegotiateProtocol, 2) \
   STATE_ENTRY(WaitForSessionSetupResponse, SessionSetup, 2) \
   STATE_ENTRY(WaitForServiceDiscoveryResponse, ServiceDiscovery, 2) \
   STATE_ENTRY(WaitForServicePaymentSelectionResponse, PaymentSelection, 2) \
   STATE_ENTRY(WaitForContractAuthenticationResponse, ContractAuthentication, 2) \
   STATE_ENTRY(WaitForChargeParameterDiscoveryResponse, ChargeParameterDiscovery, 5) /* On some charger models, the chargeParameterDiscovery needs more than a second. Wait at least 5s. */ \
   STATE_ENTRY(WaitForConnectorLock, ConnectorLock, 2) \
   STATE_ENTRY(WaitForCableCheckResponse, CableCheck, 30) \
   STATE_ENTRY(WaitForPreChargeResponse, PreCharge, 30) \
   STATE_ENTRY(WaitForContactorsClosed, ContactorsClosed, 5) \
   STATE_ENTRY(WaitForPowerDeliveryResponse, PowerDelivery, 6) /* PowerDelivery may need some time. Wait at least 6s. On Compleo charger, observed more than 1s until response. specified performance time is 4.5s (ISO) */\
   STATE_ENTRY(WaitForCurrentDemandResponse, CurrentDemand, 5) /* Test with 5s timeout. Just experimental. The specified performance time is 25ms (ISO), the specified timeout 250ms. */\
   STATE_ENTRY(WaitForCurrentDownAfterStateB, WaitCurrentDown, 0) \
   STATE_ENTRY(WaitForWeldingDetectionResponse, WeldingDetection, 2) \
   STATE_ENTRY(WaitForSessionStopResponse, SessionStop, 2) \
   STATE_ENTRY(UnrecoverableError, Error, 0) \
   STATE_ENTRY(SequenceTimeout, Timeout, 0) \
   STATE_ENTRY(SafeShutDownWaitForChargerShutdown, WaitForChargerShutdown, 0) \
   STATE_ENTRY(SafeShutDownWaitForContactorsOpen, WaitForContactorsOpen, 0) \
   STATE_ENTRY(End, End, 0)

//States enum
#define STATE_ENTRY(name, fname, timeout) PEV_STATE_##name,
enum pevstates {
STATE_LIST
};
#undef STATE_ENTRY

//state function prototypes
#define STATE_ENTRY(name, fname, timeout) static void stateFunction##name();
STATE_LIST
#undef STATE_ENTRY

//State function array
#define STATE_ENTRY(name, fname, timeout) stateFunction##name,
static void(*const stateFunctions[])() = {
STATE_LIST
};
#undef STATE_ENTRY

//Timeout array
#define STATE_ENTRY(name, fname, timeout) timeout * 33,
static const uint16_t timeouts[] = {
STATE_LIST
};
#undef STATE_ENTRY

//Enum string for data module
#define STATE_ENTRY(name, fname, timeout) __COUNTER__=fname,
const char* pevSttString = STRINGIFY(STATE_LIST);
#undef STATE_ENTRY

//String array for logging
#define STATE_ENTRY(name, fname, timeout) #fname,
const char pevSttLabels[][MAX_LABEL_LEN] = { STATE_LIST };
#undef STATE_ENTRY

#define MAX_VOLTAGE_TO_FINISH_WELDING_DETECTION 40 /* 40V is considered to be sufficiently low to not harm. The Ioniq already finishes at 65V. */
#define MAX_NUMBER_OF_WELDING_DETECTION_ROUNDS 10 /* The process time is specified with 1.5s. Ten loops should be fine. */

#define LEN_OF_EVCCID 6 /* The EVCCID is the MAC according to spec. Ioniq uses exactly these 6 byte. */


static const uint8_t exiDemoSupportedApplicationProtocolRequestIoniq[]= {0x80, 0x00, 0xdb, 0xab, 0x93, 0x71, 0xd3, 0x23, 0x4b, 0x71, 0xd1, 0xb9, 0x81, 0x89, 0x91, 0x89, 0xd1, 0x91, 0x81, 0x89, 0x91, 0xd2, 0x6b, 0x9b, 0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00, 0x00, 0x04, 0x00, 0x40 };


/* From mhpev_EVSE_Kia_Ev6_20230921_02.log we can take a SupportedApplicationProtocolRequest which announces
   DIN and ISO 2013 (aka ISO 1, according to https://openv2g.sourceforge.net/)
[13188ms] [EVSE] In state WaitForSupportedApplicationProtocolRequest, received (76bytes) = 01 FE 80 01 00 00 00 44 80 00 DB AB 93 71 D3 23 4B 71 D1 B9 81 89 91 89 D1 91 81 89 91 D2 6B 9B 3A 23 2B 30 02 00 00 04 00 01 D7 57 26 E3 A6 97 36 F3 A3 13 53 13 13 83 A3 23 A3 23 03 13 33 A4 D7 36 74 46 56 60 04 00 00 10 08 80 
[13217ms] [EVSE] {
"msgName": "supportedAppProtocolReq",
"info": "68 bytes to convert", 
"error": "",
"result": "Vehicle supports 2 protocols. ProtocolEntry#1 ProtocolNamespace=urn:din:70121:2012:MsgDef Version=2.0 SchemaID=1 Priority=1 ProtocolEntry#2 ProtocolNamespace=urn:iso:15118:2:2013:MsgDef Version=2.0 SchemaID=2 Priority=2 ",
"schema": "appHandshake",
"AppProtocol_arrayLen": "2",
"NameSpace_0": "urn:din:70121:2012:MsgDef",
"Version_0": "2.0",
"SchemaID_0": "1",
"Priority_0": "1",
"NameSpace_1": "urn:iso:15118:2:2013:MsgDef",
"Version_1": "2.0",
"SchemaID_1": "2",
"Priority_1": "2",
"debug": ""
}
[13218ms] [EVSE] The car supports 2 schemas.
[13218ms] [EVSE] The NameSpace urn:din:70121:2012:MsgDef has SchemaID 1
[13218ms] [EVSE] The NameSpace urn:iso:15118:2:2013:MsgDef has SchemaID 2
*/
static const uint8_t exiSupportedApplicationProtocolRequestDinAndIso2013[]= {
    0x80, 0x00, 0xDB, 0xAB, 0x93, 0x71, 0xD3, 0x23,
    0x4B, 0x71, 0xD1, 0xB9, 0x81, 0x89, 0x91, 0x89,
    0xD1, 0x91, 0x81, 0x89, 0x91, 0xD2, 0x6B, 0x9B,
    0x3A, 0x23, 0x2B, 0x30, 0x02, 0x00, 0x00, 0x04,
    0x00, 0x01, 0xD7, 0x57, 0x26, 0xE3, 0xA6, 0x97,
    0x36, 0xF3, 0xA3, 0x13, 0x53, 0x13, 0x13, 0x83,
    0xA3, 0x23, 0xA3, 0x23, 0x03, 0x13, 0x33, 0xA4,
    0xD7, 0x36, 0x74, 0x46, 0x56, 0x60, 0x04, 0x00,
    0x00, 0x10, 0x08, 0x80 };
    
    
static const uint8_t exiIso2013only[] = {
   /* ISO2013 only, "urn:iso:15118:2:2013:MsgDef".
     Created with openV2Gx, $ ./OpenV2G.exe EH__1
   */
    0x80, 0x00, 0xeb, 0xab, 0x93, 0x71, 0xd3, 0x4b,
    0x9b, 0x79, 0xd1, 0x89, 0xa9, 0x89, 0x89, 0xc1,
    0xd1, 0x91, 0xd1, 0x91, 0x81, 0x89, 0x99, 0xd2,
    0x6b, 0x9b, 0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00,
    0x00, 0x00, 0x00, 0x40 };

static uint16_t pev_cyclesInState;
static uint8_t pev_DelayCycles;
static pevstates pev_state=PEV_STATE_NotYetInitialized;
static uint8_t pev_isUserStopRequestOnCarSide=0;
static uint8_t pev_isUserStopRequestOnChargerSide=0;
static uint16_t pev_numberOfContractAuthenticationReq;
static uint16_t pev_numberOfChargeParameterDiscoveryReq;
static uint16_t pev_numberOfCableCheckReq;
static uint8_t pev_wasPowerDeliveryRequestedOn;
static uint8_t pev_isBulbOn;
static uint16_t pev_cyclesLightBulbDelay;
static float EVSEPresentVoltage;
static uint16_t EVSEMinimumVoltage;
static uint8_t numberOfWeldingDetectionRounds;

/***local function prototypes *****************************************/

static uint8_t pev_isTooLong(void);
static void pev_enterState(pevstates n);

/*** functions ********************************************************/

static float combineValueAndMultiplier(int32_t val, int8_t multiplier)
{
   float x;
   x = val;
   while (multiplier>0)
   {
      x=x*10;
      multiplier--;
   }
   while (multiplier<0)
   {
      x=x/10;
      multiplier++;
   }
   return x;
}

static float combineValueAndMultiplier(dinPhysicalValueType v)
{
   return combineValueAndMultiplier(v.Value, v.Multiplier);
}

static void addV2GTPHeaderAndTransmit(const uint8_t *exiBuffer, uint8_t exiBufferLen)
{
   // takes the bytearray with exidata, and adds a header to it, according to the Vehicle-to-Grid-Transport-Protocol
   // V2GTP header has 8 bytes
   // 1 byte protocol version
   // 1 byte protocol version inverted
   // 2 bytes payload type
   // 4 byte payload length
   tcpPayload[0] = 0x01; // version
   tcpPayload[1] = 0xfe; // version inverted
   tcpPayload[2] = 0x80; // payload type. 0x8001 means "EXI data"
   tcpPayload[3] = 0x01; //
   tcpPayload[4] = (uint8_t)(exiBufferLen >> 24); // length 4 byte.
   tcpPayload[5] = (uint8_t)(exiBufferLen >> 16);
   tcpPayload[6] = (uint8_t)(exiBufferLen >> 8);
   tcpPayload[7] = (uint8_t)exiBufferLen;
   if (exiBufferLen+8<TCP_PAYLOAD_LEN)
   {
      memcpy(&tcpPayload[8], exiBuffer, exiBufferLen);
      tcpPayloadLen = 8 + exiBufferLen; /* 8 byte V2GTP header, plus the EXI data */
      tcp_transmit();
   }
   else
   {
      addToTrace(MOD_PEV, "Error: EXI does not fit into tcpPayload.");
   }
}

static void encodeAndTransmit(void)
{
   /* calls the EXI encoder, adds the V2GTP header and sends the result to ethernet */
   //addToTrace("before: g_errn=" + String(g_errn));
   //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
   global_streamEncPos = 0;
   projectExiConnector_encode_DinExiDocument();
   //addToTrace("after: g_errn=" + String(g_errn));
   //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
   addV2GTPHeaderAndTransmit(global_streamEnc.data, global_streamEncPos);
}

static void routeDecoderInputData(void)
{
   /* connect the data from the TCP to the exiDecoder */
   /* The TCP receive data consists of two parts: 1. The V2GTP header and 2. the EXI stream.
      The decoder wants only the EXI stream, so we skip the V2GTP header.
      In best case, we would check also the consistency of the V2GTP header here.
   */
   global_streamDec.data = &tcp_rxdata[V2GTP_HEADER_SIZE];
   global_streamDec.size = tcp_rxdataLen - V2GTP_HEADER_SIZE;
#ifdef VERBOSE_EXI_DECODER
   showAsHex(global_streamDec.data, global_streamDec.size, "decoder will see");
#endif
   /* We have something to decode, this is a good sign that the connection is fine.
      Inform the ConnectionManager that everything is fine. */
   connMgr_ApplOk(10);
}

/********* EXI creation functions ************************/

static void pev_sendSessionSetupRequest(void) {
    uint8_t i;
    projectExiConnector_prepare_DinExiDocument();
    ExiDocEnc.dinD.V2G_Message.Body.SessionSetupReq_isUsed = 1u;
    init_dinSessionSetupReqType(&ExiDocEnc.dinD.V2G_Message.Body.SessionSetupReq);
    /* In the session setup request, the session ID zero means: create a new session.
     The format (len 8, all zero) is taken from the original Ioniq behavior. */
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[0] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[1] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[2] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[3] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[4] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[5] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[6] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytes[7] = 0;
    ExiDocEnc.dinD.V2G_Message.Header.SessionID.bytesLen = 8;
    /* Takeover from https://github.com/uhi22/OpenV2Gx/commit/fc46c3ca802f08c57120a308f69fb4d1ce14f6b6 */
    /* The EVCCID. In the ISO they write, that this shall be the EVCC MAC. But the DIN
    reserves 8 bytes (dinSessionSetupReqType_EVCCID_BYTES_SIZE is 8). This does not match.
    The Ioniq (DIN) sets the bytesLen=6 and fills the 6 bytes with its own MAC. Let's assume this
    is the best way. */
    for (i=0; i<LEN_OF_EVCCID; i++)
    {
    ExiDocEnc.dinD.V2G_Message.Body.SessionSetupReq.EVCCID.bytes[i] = getOurMac()[i];
    }
    ExiDocEnc.dinD.V2G_Message.Body.SessionSetupReq.EVCCID.bytesLen = LEN_OF_EVCCID;
    encodeAndTransmit();
}


static void pev_sendChargeParameterDiscoveryReq(void)
{
   struct dinDC_EVChargeParameterType *cp;
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.ChargeParameterDiscoveryReq_isUsed = 1u;
   init_dinChargeParameterDiscoveryReqType(&ExiDocEnc.dinD.V2G_Message.Body.ChargeParameterDiscoveryReq);
   ExiDocEnc.dinD.V2G_Message.Body.ChargeParameterDiscoveryReq.EVRequestedEnergyTransferType = dinEVRequestedEnergyTransferType_DC_extended;
   cp = &ExiDocEnc.dinD.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter;
   cp->DC_EVStatus.EVReady = 0;  /* What ever this means. The Ioniq sends 0 here in the ChargeParameterDiscoveryReq message. */
   //cp->DC_EVStatus.EVCabinConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
   //cp->DC_EVStatus.RESSConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
   cp->DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc();
   cp->EVMaximumCurrentLimit.Value = Param::GetInt(Param::MaxCurrent);
   cp->EVMaximumCurrentLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   cp->EVMaximumCurrentLimit.Unit_isUsed = 1;
   cp->EVMaximumCurrentLimit.Unit = dinunitSymbolType_A;

   cp->EVMaximumPowerLimit_isUsed = 1; /* The Ioniq sends 1 here. */
   cp->EVMaximumPowerLimit.Value = Param::GetInt(Param::MaxPower) * 10; /* maxpower is kW, then x10 x 100 by Multiplier */
   cp->EVMaximumPowerLimit.Multiplier = 2; /* 10^2 */
   cp->EVMaximumPowerLimit.Unit_isUsed = 1;
   cp->EVMaximumPowerLimit.Unit = dinunitSymbolType_W; /* Watt */

   cp->EVMaximumVoltageLimit.Value = Param::GetInt(Param::MaxVoltage);
   cp->EVMaximumVoltageLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   cp->EVMaximumVoltageLimit.Unit_isUsed = 1;
   cp->EVMaximumVoltageLimit.Unit = dinunitSymbolType_V;

   cp->EVEnergyCapacity_isUsed = 1;
   cp->EVEnergyCapacity.Value = 10000; /* Lets make it 100 kWh so it doesn't get in the way */
   cp->EVEnergyCapacity.Multiplier = 1;
   cp->EVEnergyCapacity.Unit_isUsed = 1;
   cp->EVEnergyCapacity.Unit = dinunitSymbolType_Wh; /* from Ioniq */

   cp->EVEnergyRequest_isUsed = 1;
   cp->EVEnergyRequest.Value = 10000; /* Lets make it 100 kWh so it doesn't get in the way */
   cp->EVEnergyRequest.Multiplier = 1;
   cp->EVEnergyRequest.Unit_isUsed = 1;
   cp->EVEnergyRequest.Unit = dinunitSymbolType_Wh; /* 9 from Ioniq */

   cp->FullSOC_isUsed = 1;
   cp->FullSOC = 100;
   cp->BulkSOC_isUsed = 1;
   cp->BulkSOC = 80;

   ExiDocEnc.dinD.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter_isUsed = 1;
   encodeAndTransmit();
}

static void pev_sendCableCheckReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.CableCheckReq_isUsed = 1u;
   init_dinCableCheckReqType(&ExiDocEnc.dinD.V2G_Message.Body.CableCheckReq);
#define st ExiDocEnc.dinD.V2G_Message.Body.CableCheckReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc(); /* Scaling is 1%. */
#undef st
   encodeAndTransmit();
   /* Since the response to the CableCheckRequest may need longer, inform the connection manager to be patient.
      This makes sure, that the timeout of the state machine comes before the timeout of the connectionManager, so
      that we enter the safe shutdown sequence as intended.
      (This is a takeover from https://github.com/uhi22/pyPLC/commit/08af8306c60d57c4c33221a0dbb25919371197f9 ) */
   connMgr_ApplOk(31);
   hardwareInterface_LogTheCpPpPhysicalData(); /* for trouble shooting, log some physical values to the console */
}

static void pev_sendPreChargeReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.PreChargeReq_isUsed = 1u;
   init_dinPreChargeReqType(&ExiDocEnc.dinD.V2G_Message.Body.PreChargeReq);
#define st ExiDocEnc.dinD.V2G_Message.Body.PreChargeReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc(); /* The SOC. Scaling is 1%. */
#undef st
#define tvolt ExiDocEnc.dinD.V2G_Message.Body.PreChargeReq.EVTargetVoltage
   tvolt.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   tvolt.Unit = dinunitSymbolType_V;
   tvolt.Unit_isUsed = 1;
   tvolt.Value = hardwareInterface_getAccuVoltage(); /* The precharge target voltage. Scaling is 1V. */
#undef tvolt
#define tcurr ExiDocEnc.dinD.V2G_Message.Body.PreChargeReq.EVTargetCurrent
   tcurr.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   tcurr.Unit = dinunitSymbolType_A;
   tcurr.Unit_isUsed = 1;
   tcurr.Value = 1; /* 1A for precharging */
#undef tcurr
   encodeAndTransmit();
}

static void pev_sendPowerDeliveryReq(uint8_t isOn)
{
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq_isUsed = 1u;
   init_dinPowerDeliveryReqType(&ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq);
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.ReadyToChargeState = isOn; /* 1=ON, 0=OFF */
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter_isUsed = 1;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVReady = 1; /* 1 means true. We are ready. */
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc();
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.ChargingComplete = 0; /* boolean. Charging not finished. */
   /* some "optional" fields seem to be mandatory, at least the Ioniq sends them, and the Compleo charger ignores the message if too short.
      See https://github.com/uhi22/OpenV2Gx/commit/db2c7addb0cae0e16175d666e736efd551f3e14d#diff-333579da65917bc52ef70369b576374d0ee5dbca47d2b1e3bedb6f062decacff
      Let's fill them:
   */
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVCabinConditioning_isUsed = 1;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVCabinConditioning = 0;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSConditioning_isUsed = 1;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSConditioning = 0;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.BulkChargingComplete_isUsed  = 1;
   ExiDocEnc.dinD.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.BulkChargingComplete = 0;
   encodeAndTransmit();
}

static void pev_sendCurrentDemandReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq_isUsed = 1u;
   init_dinCurrentDemandReqType(&ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq);
   // DC_EVStatus
#define st ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc();
#undef st
   // EVTargetVoltage
#define tvolt ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.EVTargetVoltage
   tvolt.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   tvolt.Unit = dinunitSymbolType_V;
   tvolt.Unit_isUsed = 1;
   tvolt.Value = hardwareInterface_getChargingTargetVoltage(); /* The charging target. Scaling is 1V. */
#undef tvolt
   // EVTargetCurrent
#define tcurr ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.EVTargetCurrent
   tcurr.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   tcurr.Unit = dinunitSymbolType_A;
   tcurr.Unit_isUsed = 1;
   tcurr.Value = hardwareInterface_getChargingTargetCurrent(); /* The charging target current. Scaling is 1A. */
#undef tcurr
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.ChargingComplete = 0; /* boolean. Todo: Do we need to take this from command line? Or is it fine
    that the PEV just sends a PowerDeliveryReq with STOP, if it decides to stop the charging? */
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete_isUsed = 1u;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete = 0u; /* not complete */
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC_isUsed = 1u;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit = dinunitSymbolType_s;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit_isUsed = 1;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Value = 1200; /* seconds */

   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC_isUsed = 1u;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit = dinunitSymbolType_s;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit_isUsed = 1;
   ExiDocEnc.dinD.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Value = 600; /* seconds */
   encodeAndTransmit();
}

static void pev_sendWeldingDetectionReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   ExiDocEnc.dinD.V2G_Message.Body.WeldingDetectionReq_isUsed = 1u;
   init_dinWeldingDetectionReqType(&ExiDocEnc.dinD.V2G_Message.Body.WeldingDetectionReq);
#define st ExiDocEnc.dinD.V2G_Message.Body.WeldingDetectionReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc();
#undef st
   encodeAndTransmit();
}

/**** State functions ***************/
//Empty functions
static void stateFunctionNotYetInitialized() {}
static void stateFunctionConnecting() {}

static void stateFunctionConnected(void)
{
   // We have a freshly established TCP channel. We start the V2GTP/EXI communication now.
   // We just use the initial request message from the Ioniq. It contains one entry: DIN.
   addToTrace(MOD_PEV, "Checkpoint400: Sending the initial SupportedApplicationProtocolReq");
   setCheckpoint(400);
   if (Param::GetInt(Param::PlcSchema) == 0) { /* DIN protocol */
     addToTrace(MOD_PEV, "Announcing DIN schema");
     addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));
   } else if (Param::GetInt(Param::PlcSchema) == 1) { /* ISO protocol */
        addToTrace(MOD_PEV, "Announcing ISO schema");
        /* SupportedApplicationProtocolRequest for ISO 2013 */
        #ifdef USE_ISO1
          addToTrace(MOD_PEV, "Announcing ISO2013 (aka ISO1) schema");
          addV2GTPHeaderAndTransmit(exiIso2013only, sizeof(exiIso2013only));
        #endif
        #ifdef USE_ISO2
        #endif
   } else { /* DIN and ISO protocol */
        /* the SupportedApplicationProtocolRequest for DIN+ISO */
        addToTrace(MOD_PEV, "Announcing DIN and ISO2013 (aka ISO1) schema");
        addV2GTPHeaderAndTransmit(exiSupportedApplicationProtocolRequestDinAndIso2013, sizeof(exiSupportedApplicationProtocolRequestDinAndIso2013));
   }
   hardwareInterface_resetSimulation();
   Param::SetInt(Param::StopReason, STOP_REASON_NONE);
   pev_enterState(PEV_STATE_WaitForSupportedApplicationProtocolResponse);
}

static void stateFunctionWaitForSupportedApplicationProtocolResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForSupportedApplicationProtocolResponse");
      routeDecoderInputData();
      projectExiConnector_decode_appHandExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (aphsDoc.supportedAppProtocolRes_isUsed)
      {
         /* it is the correct response */
         addToTrace(MOD_PEV, "supportedAppProtocolRes");
         printf("ResponseCode %d, SchemaID_isUsed %d, SchemaID %d\r\n",
                aphsDoc.supportedAppProtocolRes.ResponseCode,
                aphsDoc.supportedAppProtocolRes.SchemaID_isUsed,
                aphsDoc.supportedAppProtocolRes.SchemaID);
         /* Here we distinguish between DIN and ISO */
         if ((Param::GetInt(Param::PlcSchema) == 0) && (aphsDoc.supportedAppProtocolRes.SchemaID_isUsed==1) && (aphsDoc.supportedAppProtocolRes.SchemaID == 1)) {
             /* DIN only was requested and confirmed. We used the request from the Ioniq, which in DIN only and uses SchemaID 1 */
            publishStatus("Schema negotiated", "");
            addToTrace(MOD_PEV, "Checkpoint403: Schema DIN negotiated. And Checkpoint500: Will send SessionSetupReq");
            setCheckpoint(500);
            pev_sendSessionSetupRequest();
            pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
         } else if ((Param::GetInt(Param::PlcSchema) == 1) && (aphsDoc.supportedAppProtocolRes.SchemaID_isUsed==1) && (aphsDoc.supportedAppProtocolRes.SchemaID == 0)) {
             /* ISO only was requested and confirmed. The self-made request uses SchemaID 0 */
            publishStatus("Schema negotiated", "");
            #ifdef USE_ISO1
              addToTrace(MOD_PEV, "Checkpoint403: Schema ISO1 negotiated.");
              pevStateMachineISO1_Start(); /* Start the ISO1 state machine */
            #endif
            #ifdef USE_ISO2
              addToTrace(MOD_PEV, "Checkpoint403: Schema ISO2 negotiated.");
              pevStateMachineISO2_Start(); /* Start the ISO2 state machine */
            #endif
            pev_enterState(PEV_STATE_End); /* nothing more to do here */
         } else if ((Param::GetInt(Param::PlcSchema) == 2) && (aphsDoc.supportedAppProtocolRes.SchemaID_isUsed==1)) {
             /* we requested DIN+ISO, and the charger should decide for one of those. */
             if (aphsDoc.supportedAppProtocolRes.SchemaID == 1) {
                /* The charger decided to use Schema 1, which we announced as DIN */
                addToTrace(MOD_PEV, "Checkpoint403: Schema DIN negotiated (decided by charger). And Checkpoint500: Will send SessionSetupReq");
                setCheckpoint(500);
                pev_sendSessionSetupRequest();
                pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
             } else if (aphsDoc.supportedAppProtocolRes.SchemaID == 2) {
                /* The charger decided to use Schema 2, which we announced as ISO */
                #ifdef USE_ISO1
                  addToTrace(MOD_PEV, "Checkpoint403: Schema ISO1 negotiated (decided by charger).");
                  pevStateMachineISO1_Start(); /* Start the ISO1 state machine */
                #endif
                pev_enterState(PEV_STATE_End); /* nothing more to do here */
             } else {
                addToTrace(MOD_PEV, "Error: Charger decided for a schema which we did not announce.");
             }
         }
      }
   }
   if (pev_isTooLong())
      ErrorMessage::Post(ERR_PLCTIMEOUT);
}

static void stateFunctionWaitForSessionSetupResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForSessionSetupResponse");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      //addToTrace("after decoding: g_errn=" + String(g_errn));
      //addToTrace("global_streamDecPos=" + String(global_streamDecPos));
      if (ExiDocDec.dinD.V2G_Message.Body.SessionSetupRes_isUsed)
      {
         memcpy(sessionId, ExiDocDec.dinD.V2G_Message.Header.SessionID.bytes, SESSIONID_LEN);
         sessionIdLen = ExiDocDec.dinD.V2G_Message.Header.SessionID.bytesLen; /* store the received SessionID, we will need it later. */
         addToTrace(MOD_PEV, "Checkpoint506: The Evse decided for SessionId", sessionId, sessionIdLen);
         setCheckpoint(506);
         publishStatus("Session established", "");
         addToTrace(MOD_PEV, "Will send ServiceDiscoveryReq");
         projectExiConnector_prepare_DinExiDocument();
         ExiDocEnc.dinD.V2G_Message.Body.ServiceDiscoveryReq_isUsed = 1u;
         init_dinServiceDiscoveryReqType(&ExiDocEnc.dinD.V2G_Message.Body.ServiceDiscoveryReq);
         setCheckpoint(510);
         encodeAndTransmit();
         pev_enterState(PEV_STATE_WaitForServiceDiscoveryResponse);
      }
   }
}

static void stateFunctionWaitForServiceDiscoveryResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForServiceDiscoveryResponse");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.ServiceDiscoveryRes_isUsed)
      {
         publishStatus("ServDisc done", "");
         addToTrace(MOD_PEV, "Will send ServicePaymentSelectionReq");
         projectExiConnector_prepare_DinExiDocument();
         ExiDocEnc.dinD.V2G_Message.Body.ServicePaymentSelectionReq_isUsed = 1u;
         init_dinServicePaymentSelectionReqType(&ExiDocEnc.dinD.V2G_Message.Body.ServicePaymentSelectionReq);
         /* the mandatory fields in ISO are SelectedPaymentOption and SelectedServiceList. Same in DIN. */
         ExiDocEnc.dinD.V2G_Message.Body.ServicePaymentSelectionReq.SelectedPaymentOption = dinpaymentOptionType_ExternalPayment; /* not paying per car */
         ExiDocEnc.dinD.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.array[0].ServiceID = 1; /* todo: what ever this means. The Ioniq uses 1. */
         ExiDocEnc.dinD.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.arrayLen = 1; /* just one element in the array */
         setCheckpoint(520);
         encodeAndTransmit();
         pev_enterState(PEV_STATE_WaitForServicePaymentSelectionResponse);
      }
   }
}

static void stateFunctionWaitForServicePaymentSelectionResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForServicePaymentSelectionResponse");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.ServicePaymentSelectionRes_isUsed)
      {
         publishStatus("ServPaySel done", "");
         addToTrace(MOD_PEV, "Checkpoint530: Will send ContractAuthenticationReq");
         setCheckpoint(530);
         projectExiConnector_prepare_DinExiDocument();
         ExiDocEnc.dinD.V2G_Message.Body.ContractAuthenticationReq_isUsed = 1u;
         init_dinContractAuthenticationReqType(&ExiDocEnc.dinD.V2G_Message.Body.ContractAuthenticationReq);
         /* no other fields are manatory */
         encodeAndTransmit();
         pev_numberOfContractAuthenticationReq = 1; // This is the first request.
         pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
      }
   }
}

static void stateFunctionWaitForContractAuthenticationResponse(void)
{
   if (pev_cyclesInState<30)   // The first second in the state just do nothing.
   {
      return;
   }
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForContractAuthenticationResponse");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.ContractAuthenticationRes_isUsed)
      {
         // In normal case, we can have two results here: either the Authentication is needed (the user
         // needs to authorize by RFID card or app, or something like this.
         // Or, the authorization is finished. This is shown by EVSEProcessing=Finished.
         if (ExiDocDec.dinD.V2G_Message.Body.ContractAuthenticationRes.EVSEProcessing == dinEVSEProcessingType_Finished)
         {
            publishStatus("Auth finished", "");
            addToTrace(MOD_PEV, "Checkpoint538 and 540: Auth is Finished. Will send ChargeParameterDiscoveryReq");
            setCheckpoint(540);
            pev_sendChargeParameterDiscoveryReq();
            pev_numberOfChargeParameterDiscoveryReq = 1; // first message
            pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse);
         }
         else
         {
            // Not (yet) finished.
            if (pev_numberOfContractAuthenticationReq>=120)   // approx 120 seconds, maybe the user searches two minutes for his RFID card...
            {
               addToTrace(MOD_PEV, "Authentication lasted too long. Giving up.");
               pev_enterState(PEV_STATE_SequenceTimeout);
            }
            else
            {
               // Try again.
               pev_numberOfContractAuthenticationReq += 1; // count the number of tries.
               publishStatus("Waiting f Auth", "");
               //addToTrace("Not (yet) finished. Will again send ContractAuthenticationReq #" + String(pev_numberOfContractAuthenticationReq));
               addToTrace(MOD_PEV, "Not (yet) finished. Will again send ContractAuthenticationReq");
               encodeAndTransmit();
               // We just stay in the same state, until the timeout elapses.
               pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
            }
         }
      }
   }
}

static void stateFunctionWaitForChargeParameterDiscoveryResponse(void)
{
   if (pev_cyclesInState<30)   // The first second in the state just do nothing.
   {
      return;
   }
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForChargeParameterDiscoveryResponse");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.ChargeParameterDiscoveryRes_isUsed)
      {
         // We can have two cases here:
         // (A) The charger needs more time to show the charge parameters.
         // (B) The charger finished to tell the charge parameters.
         if (ExiDocDec.dinD.V2G_Message.Body.ChargeParameterDiscoveryRes.EVSEProcessing == dinEVSEProcessingType_Finished)
         {
            publishStatus("ChargeParams discovered", "");
            addToTrace(MOD_PEV, "Checkpoint550: ChargeParams are discovered.. Will change to state C.");
#define dcparm ExiDocDec.dinD.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter
            float evseMaxVoltage = combineValueAndMultiplier(dcparm.EVSEMaximumVoltageLimit);
            float evseMaxCurrent = combineValueAndMultiplier(dcparm.EVSEMaximumCurrentLimit);
            EVSEMinimumVoltage = combineValueAndMultiplier(dcparm.EVSEMinimumVoltageLimit);
#undef dcparm
            Param::SetFloat(Param::EvseMaxVoltage, evseMaxVoltage);
            Param::SetFloat(Param::EvseMaxCurrent, evseMaxCurrent);

            setCheckpoint(550);
            // pull the CP line to state C here:
            hardwareInterface_setStateC();
            addToTrace(MOD_PEV, "Checkpoint555: Locking the connector.");
            hardwareInterface_triggerConnectorLocking();
            //If we are not ready for charging, don't go past this state -> will time out
            if (hardwareInterface_stopChargeRequested())
            {
               addToTrace(MOD_PEV, "Stopping due to stoprequest.");
               pev_enterState(PEV_STATE_WaitForServiceDiscoveryResponse);
            }
            else
            {
               pev_enterState(PEV_STATE_WaitForConnectorLock);
            }
         }
         else
         {
            // Not (yet) finished.
            if (pev_numberOfChargeParameterDiscoveryReq>=60)
            {
               /* approx 60 seconds, should be sufficient for the charger to find its parameters.
                   ... The ISO allows up to 55s reaction time and 60s timeout for "ongoing". Taken over from
                       https://github.com/uhi22/pyPLC/commit/01c7c069fd4e7b500aba544ae4cfce6774f7344a */
               //addToTrace("ChargeParameterDiscovery lasted too long. " + String(pev_numberOfChargeParameterDiscoveryReq) + " Giving up.");
               addToTrace(MOD_PEV, "ChargeParameterDiscovery lasted too long. Giving up.");
               pev_enterState(PEV_STATE_SequenceTimeout);
            }
            else
            {
               // Try again.
               pev_numberOfChargeParameterDiscoveryReq += 1; // count the number of tries.
               publishStatus("disc ChargeParams", "");
               //addToTrace("Not (yet) finished. Will again send ChargeParameterDiscoveryReq #" + String(pev_numberOfChargeParameterDiscoveryReq));
               addToTrace(MOD_PEV, "Not (yet) finished. Will again send ChargeParameterDiscoveryReq");
               pev_sendChargeParameterDiscoveryReq();
               // we stay in the same state
               pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse);
            }
         }
      }
   }
}

static void stateFunctionWaitForConnectorLock(void)
{
   if (hardwareInterface_isConnectorLocked())
   {
      addToTrace(MOD_PEV, "Checkpoint560: Connector Lock confirmed. Will send CableCheckReq.");
      setCheckpoint(560);
      pev_sendCableCheckReq();
      pev_numberOfCableCheckReq = 1; // This is the first request.
      pev_enterState(PEV_STATE_WaitForCableCheckResponse);
   }
   if (pev_isTooLong())
      ErrorMessage::Post(ERR_LOCKTIMEOUT);
}

static void stateFunctionWaitForCableCheckResponse(void)
{
   uint8_t rc, proc;
   if (pev_cyclesInState<30)   // The first second in the state just do nothing.
   {
      return;
   }
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      //addToTrace(MOD_PEV, "In state WaitForCableCheckResponse, received:");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.CableCheckRes_isUsed)
      {
         rc = ExiDocDec.dinD.V2G_Message.Body.CableCheckRes.ResponseCode;
         proc = ExiDocDec.dinD.V2G_Message.Body.CableCheckRes.EVSEProcessing;
         Param::SetInt(Param::EvseVoltage, 0);
         //addToTrace("The CableCheck result is " + String(rc) + " " + String(proc));
         // We have two cases here:
         // 1) The charger says "cable check is finished and cable ok", by setting ResponseCode=OK and EVSEProcessing=Finished.
         // 2) Else: The charger says "need more time or cable not ok". In this case, we just run into timeout and start from the beginning.
         if ((proc==dinEVSEProcessingType_Finished) && (rc==dinresponseCodeType_OK))
         {
            publishStatus("CbleChck done", "");
            addToTrace(MOD_PEV, "The EVSE says that the CableCheck is finished and ok.");
            addToTrace(MOD_PEV, "Will send PreChargeReq");
            setCheckpoint(570);
            pev_sendPreChargeReq();
            connMgr_ApplOk(31); /* PreChargeResponse may need longer. Inform the connection manager to be patient.
                                (This is a takeover from https://github.com/uhi22/pyPLC/commit/08af8306c60d57c4c33221a0dbb25919371197f9 ) */
            pev_enterState(PEV_STATE_WaitForPreChargeResponse);
         }
         else
         {
            if (pev_numberOfCableCheckReq>60)   /* approx 60s should be sufficient for cable check. The ISO allows up to 55s reaction time and 60s timeout for "ongoing". Taken over from https://github.com/uhi22/pyPLC/commit/01c7c069fd4e7b500aba544ae4cfce6774f7344a */
            {
               //addToTrace("CableCheck lasted too long. " + String(pev_numberOfCableCheckReq) + " Giving up.");
               addToTrace(MOD_PEV, "CableCheck lasted too long. Giving up.");
               pev_enterState(PEV_STATE_SequenceTimeout);
            }
            else
            {
               // cable check not yet finished or finished with bad result -> try again
               pev_numberOfCableCheckReq += 1;
               publishStatus("CbleChck ongoing", "");
               addToTrace(MOD_PEV, "Will again send CableCheckReq");
               pev_sendCableCheckReq();
               // stay in the same state
               pev_enterState(PEV_STATE_WaitForCableCheckResponse);
            }
         }
      }
   }
}

static void stateFunctionWaitForPreChargeResponse(void)
{
   hardwareInterface_simulatePreCharge();
   if (pev_DelayCycles>0)
   {
      pev_DelayCycles-=1;
      return;
   }
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      //addToTrace(MOD_PEV, "In state WaitForPreChargeResponse, received:");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.PreChargeRes_isUsed)
      {
         addToTrace(MOD_PEV, "PreCharge aknowledge received.");
         EVSEPresentVoltage = combineValueAndMultiplier(ExiDocDec.dinD.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage);
         Param::SetFloat(Param::EvseVoltage, EVSEPresentVoltage);

         uint16_t inletVtg = hardwareInterface_getInletVoltage();
         uint16_t batVtg = hardwareInterface_getAccuVoltage();

         if (Param::GetInt(Param::logging) & MOD_PEV) {
             printf("PreCharge aknowledge received. Inlet %dV, accu %dV, uMin %dV\r\n", inletVtg, batVtg, EVSEMinimumVoltage);
         }
         if ((ABS(inletVtg - batVtg) < PARAM_U_DELTA_MAX_FOR_END_OF_PRECHARGE) && (batVtg > EVSEMinimumVoltage))
         {
            addToTrace(MOD_PEV, "Difference between accu voltage and inlet voltage is small. Sending PowerDeliveryReq.");
            publishStatus("PreCharge done", "");
            if (isLightBulbDemo)
            {
               // For light-bulb-demo, nothing to do here.
               addToTrace(MOD_PEV, "This is a light bulb demo. Do not turn-on the relay at end of precharge.");
            }
            else
            {
               // In real-world-case, turn the power relay on.
               hardwareInterface_setPowerRelayOn();
            }
            pev_wasPowerDeliveryRequestedOn=1;
            setCheckpoint(600);
            pev_sendPowerDeliveryReq(1); /* 1 is ON */
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
         }
         else
         {
            publishStatus("PreChrge ongoing", String(u) + "V");
            addToTrace(MOD_PEV, "Difference too big. Continuing PreCharge.");
            pev_sendPreChargeReq();
            pev_DelayCycles=15; // wait with the next evaluation approx half a second
         }
      }
   }
   if (pev_isTooLong())
      ErrorMessage::Post(ERR_PRECTIMEOUT);
}

static void stateFunctionWaitForContactorsClosed(void)
{
   uint8_t readyForNextState=0;
   if (pev_DelayCycles>0)
   {
      pev_DelayCycles--;
      return;
   }
   if (isLightBulbDemo)
   {
      readyForNextState=1; /* if it's just a bulb demo, we do not wait for contactor, because it is not switched in this moment. */
   }
   else
   {
      readyForNextState = hardwareInterface_getPowerRelayConfirmation(); /* check if the contactor is closed */
      if (readyForNextState)
      {
         addToTrace(MOD_PEV, "Contactors are confirmed to be closed.");
         publishStatus("Contactors ON", "");
      }
   }
   if (readyForNextState)
   {
      addToTrace(MOD_PEV, "Sending PowerDeliveryReq.");
      pev_sendPowerDeliveryReq(1); /* 1 is ON */
      pev_wasPowerDeliveryRequestedOn=1;
      pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
   }
}


static void stateFunctionWaitForPowerDeliveryResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      //addToTrace(MOD_PEV, "In state WaitForPowerDeliveryRes, received:");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.PowerDeliveryRes_isUsed)
      {
         if (pev_wasPowerDeliveryRequestedOn)
         {
            publishStatus("PwrDelvy ON success", "");
            addToTrace(MOD_PEV, "Checkpoint700: Starting the charging loop with CurrentDemandReq");
            setCheckpoint(700);
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
         }
         else
         {
            /* We requested "OFF". This is while the charging session is ending.
            When we received this response, the charger had up to 1.5s time to ramp down
            the current. On Compleo, there are really 1.5s until we get this response.
            See https://github.com/uhi22/pyPLC#detailled-investigation-about-the-normal-end-of-the-charging-session */
            publishStatus("PwrDelvry OFF success", "");
            setCheckpoint(810);
            /* set the CP line to B */
            hardwareInterface_setStateB(); /* ISO Figure 107: The PEV shall set stateB after receiving PowerDeliveryRes and before WeldingDetectionReq */
            addToTrace(MOD_PEV, "Giving the charger some time to detect StateB and ramp down the current.");
            pev_DelayCycles = 10; /* 15*30ms=450ms for charger shutdown. Should be more than sufficient, because somewhere was a requirement with 20ms between StateB until current is down. The Ioniq uses 300ms. */
            pev_enterState(PEV_STATE_WaitForCurrentDownAfterStateB); /* We give the charger some time to detect the StateB and fully ramp down
                                                             the current */
         }
      }
   }
}

static void stateFunctionWaitForCurrentDownAfterStateB(void) {
    /* During normal end of the charging session, we have set the StateB, and
       want to give the charger some time to ramp down the current completely,
       before we are opening the contactors. */
    if (pev_DelayCycles>0) {
        /* just waiting */
        pev_DelayCycles--;
    } else {
        /* Time is over. Current flow should have been stopped by the charger. Let's open the contactors and send a weldingDetectionRequest, to find out whether the voltage drops. */
        addToTrace(MOD_PEV, "Turning off the relay and starting the WeldingDetection");
        hardwareInterface_setPowerRelayOff();
        pev_isBulbOn = 0;
        setCheckpoint(850);
        /* We do not need a waiting time before sending the weldingDetectionRequest, because the weldingDetection
        will be anyway in a loop. So the first round will see a high voltage (because the contactor mechanically needed
        some time to open, but this is no problem, the next samples will see decreasing voltage in normal case. */
        numberOfWeldingDetectionRounds = 0;
        pev_sendWeldingDetectionReq();
        pev_enterState(PEV_STATE_WaitForWeldingDetectionResponse);
    }
}

static void stateFunctionWaitForCurrentDemandResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      //addToTrace(MOD_PEV, "In state WaitForCurrentDemandRes, received:");
      //showAsHex(tcp_rxdata, tcp_rxdataLen, "");
      routeDecoderInputData();
      //printf("[%d] step1 %d\r\n", rtc_get_ms(), tcp_rxdataLen);
      projectExiConnector_decode_DinExiDocument();

      //printf("[%d] step2 %d %d\r\n", rtc_get_ms(), g_errn, global_streamDecPos);

      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.CurrentDemandRes_isUsed)
      {
         /* as long as the accu is not full and no stop-demand from the user, we continue charging */
         pev_isUserStopRequestOnChargerSide=0;
         if (ExiDocDec.dinD.V2G_Message.Body.CurrentDemandRes.DC_EVSEStatus.EVSEStatusCode == dinDC_EVSEStatusCodeType_EVSE_Shutdown)
         {
            /* https://github.com/uhi22/pyPLC#example-flow, checkpoint 790: If the user stops the
               charging session on the charger, we get a CurrentDemandResponse with
               DC_EVSEStatus.EVSEStatusCode = 2 "EVSE_Shutdown" (observed on Compleo. To be tested
               on other chargers. */
            addToTrace(MOD_PEV, "Checkpoint790: Charging is terminated from charger side.");
            setCheckpoint(790);
            pev_isUserStopRequestOnChargerSide = 1;
            Param::SetInt(Param::StopReason, STOP_REASON_CHARGER_SHUTDOWN);
         }
         if (ExiDocDec.dinD.V2G_Message.Body.CurrentDemandRes.DC_EVSEStatus.EVSEStatusCode == dinDC_EVSEStatusCodeType_EVSE_EmergencyShutdown)
         {
            /* If the charger reports an emergency, we stop the charging. */
            addToTrace(MOD_PEV, "Charger reported EmergencyShutdown.");
            pev_wasPowerDeliveryRequestedOn=0;
            setCheckpoint(800);
            pev_sendPowerDeliveryReq(0);
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
            Param::SetInt(Param::StopReason, STOP_REASON_CHARGER_EMERGENCY_SHUTDOWN);
         }
         /* If the pushbutton is pressed longer than 0.5s or enable is set to off, we interpret this as charge stop request. */
         pev_isUserStopRequestOnCarSide = hardwareInterface_stopChargeRequested();
         if (hardwareInterface_getIsAccuFull() || pev_isUserStopRequestOnCarSide || pev_isUserStopRequestOnChargerSide)
         {
            if (hardwareInterface_getIsAccuFull())
            {
               publishStatus("Accu full", "");
               addToTrace(MOD_PEV, "Accu is full. Sending PowerDeliveryReq Stop.");
               Param::SetInt(Param::StopReason, STOP_REASON_ACCU_FULL);
            }
            else if (pev_isUserStopRequestOnCarSide)
            {
               publishStatus("User req stop on car side", "");
               addToTrace(MOD_PEV, "User requested stop on car side. Sending PowerDeliveryReq Stop.");
            }
            else
            {
               publishStatus("User req stop on charger side", "");
               addToTrace(MOD_PEV, "User requested stop on charger side. Sending PowerDeliveryReq Stop.");
            }
            pev_wasPowerDeliveryRequestedOn=0;
            setCheckpoint(800);
            pev_sendPowerDeliveryReq(0); /* we can immediately send the powerDeliveryStopRequest, while we are under full current.
                                            sequence explained here: https://github.com/uhi22/pyPLC#detailled-investigation-about-the-normal-end-of-the-charging-session */
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
         }
         else
         {
            /* continue charging loop */
            hardwareInterface_simulateCharging();
            EVSEPresentVoltage = combineValueAndMultiplier(ExiDocDec.dinD.V2G_Message.Body.CurrentDemandRes.EVSEPresentVoltage);
            uint16_t evsePresentCurrent = combineValueAndMultiplier(ExiDocDec.dinD.V2G_Message.Body.CurrentDemandRes.EVSEPresentCurrent);
            //publishStatus("Charging", String(u) + "V", String(hardwareInterface_getSoc()) + "%");
            Param::SetFloat(Param::EvseVoltage, EVSEPresentVoltage);
            Param::SetInt(Param::EvseCurrent, evsePresentCurrent);
            setCheckpoint(710);
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
         }
      }
   }
   if (isLightBulbDemo)
   {
      if (pev_cyclesLightBulbDelay<=33*2)
      {
         pev_cyclesLightBulbDelay+=1;
      }
      else
      {
         if (!pev_isBulbOn)
         {
            addToTrace(MOD_PEV, "This is a light bulb demo. Turning-on the bulb when 2s in the main charging loop.");
            hardwareInterface_setPowerRelayOn();
            pev_isBulbOn = 1;
         }
      }
   }
}

static void stateFunctionWaitForWeldingDetectionResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForWeldingDetectionRes");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.WeldingDetectionRes_isUsed)
      {
        /* The charger measured the voltage on the cable, and gives us the value. In the first
           round will show a quite high voltage, because the contactors are just opening. We
           need to repeat the requests, until the voltage is at a non-dangerous level. */
         EVSEPresentVoltage = combineValueAndMultiplier(ExiDocDec.dinD.V2G_Message.Body.WeldingDetectionRes.EVSEPresentVoltage);
         Param::SetFloat(Param::EvseVoltage, EVSEPresentVoltage);
         if (Param::GetInt(Param::logging) & MOD_PEV) {
             printf("EVSEPresentVoltage %dV\r\n", (int)EVSEPresentVoltage);
         }
         if (EVSEPresentVoltage<MAX_VOLTAGE_TO_FINISH_WELDING_DETECTION) {
            /* voltage is low, weldingDetection finished successfully. */
            publishStatus("WeldingDet done", "");
            addToTrace(MOD_PEV, "WeldingDetection successfully finished. Sending SessionStopReq");
            projectExiConnector_prepare_DinExiDocument();
            ExiDocEnc.dinD.V2G_Message.Body.SessionStopReq_isUsed = 1u;
            init_dinSessionStopType(&ExiDocEnc.dinD.V2G_Message.Body.SessionStopReq);
            /* no other fields are mandatory */
            setCheckpoint(900);
            encodeAndTransmit();
            addToTrace(MOD_PEV, "Unlocking the connector");
            /* unlock the connectore here. Better here than later, to avoid endless locked connector in case of broken SessionStopResponse. */
            hardwareInterface_triggerConnectorUnlocking();
            pev_enterState(PEV_STATE_WaitForSessionStopResponse);
         } else {
             /* The voltage on the cable is still high, so we make another round with the WeldingDetection. */
             if (numberOfWeldingDetectionRounds<MAX_NUMBER_OF_WELDING_DETECTION_ROUNDS) {
                 /* max number of rounds not yet reached */
                 addToTrace(MOD_PEV, "WeldingDetection: voltage still too high. Sending again WeldingDetectionReq.");
                 pev_sendWeldingDetectionReq();
                 pev_enterState(PEV_STATE_WaitForWeldingDetectionResponse);
             } else {
                 /* even after multiple welding detection requests/responses, the voltage did not fall as expected.
                 This may be due to two hanging/welded contactors or an issue of the charging station. We let the state machine
                 run into timeout and safe shutdown sequence, this will at least indicate the red light to the user. */
                 addToTrace(MOD_PEV, "WeldingDetection: ERROR: contactors probably welded. Did not reach low voltage. Entering safe shutdown.");
                 ErrorMessage::Post(ERR_RELAYWELDED);
             }
         }
      }
   }
}

static void stateFunctionWaitForSessionStopResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForSessionStopRes");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (ExiDocDec.dinD.V2G_Message.Body.SessionStopRes_isUsed)
      {
         // req -508
         // Todo: close the TCP connection here.
         publishStatus("Stopped normally", "");
         addToTrace(MOD_PEV, "Charging is finished");
         pev_enterState(PEV_STATE_End);
      }
   }
}


static void stateFunctionSequenceTimeout(void)
{
   /* Here we end, if we run into a timeout in the state machine. */
   publishStatus("ERROR Timeout", "");
   /* Initiate the safe-shutdown-sequence. */
   addToTrace(MOD_PEV, "Safe-shutdown-sequence: setting state B");
   setCheckpoint(1100);
   hardwareInterface_setStateB(); /* setting CP line to B disables in the charger the current flow. */
   pev_DelayCycles = 66; /* 66*30ms=2s for charger shutdown */
   pev_enterState(PEV_STATE_SafeShutDownWaitForChargerShutdown);
}

static void stateFunctionUnrecoverableError(void)
{
   /* Here we end, if the EVSE reported an error code, which terminates the charging session. */
   publishStatus("ERROR reported", "");
   /* Initiate the safe-shutdown-sequence. */
   addToTrace(MOD_PEV, "Safe-shutdown-sequence: setting state B");
   ErrorMessage::Post(ERR_EVSEFAULT);
   setCheckpoint(1200);
   hardwareInterface_setStateB(); /* setting CP line to B disables in the charger the current flow. */
   pev_DelayCycles = 66; /* 66*30ms=2s for charger shutdown */
   pev_enterState(PEV_STATE_SafeShutDownWaitForChargerShutdown);
}

static void stateFunctionSafeShutDownWaitForChargerShutdown(void)
{
   /* wait state, to give the charger the time to stop the current. */
   if (pev_DelayCycles>0)
   {
      pev_DelayCycles--;
      return;
   }
   /* Now the current flow is stopped by the charger. We can safely open the contactors: */
   addToTrace(MOD_PEV, "Safe-shutdown-sequence: opening contactors");
   setCheckpoint(1300);
   hardwareInterface_setPowerRelayOff();
   pev_DelayCycles = 33; /* 33*30ms=1s for opening the contactors */
   pev_enterState(PEV_STATE_SafeShutDownWaitForContactorsOpen);
}

static void stateFunctionSafeShutDownWaitForContactorsOpen(void)
{
   /* wait state, to give the contactors the time to open. */
   if (pev_DelayCycles>0)
   {
      pev_DelayCycles--;
      return;
   }
   /* Finally, when we have no current and no voltage, unlock the connector */
   setCheckpoint(1400);
   addToTrace(MOD_PEV, "Safe-shutdown-sequence: unlocking the connector");
   hardwareInterface_triggerConnectorUnlocking();
   /* This is the end of the safe-shutdown-sequence. */
   pev_enterState(PEV_STATE_End);
}

static void stateFunctionEnd(void)
{
   /* Just stay here, until we get re-initialized after a new SLAC/SDP. */
}

static void pev_enterState(pevstates n)
{
   //printf("[%d] [PEV] from %d entering %d\r\n", rtc_get_ms(), pev_state, n);
   pev_state = n;
   pev_cyclesInState = 0;
   Param::SetInt(Param::opmode, n);
}

static uint8_t pev_isTooLong(void)
{
   return timeouts[pev_state] > 0 && pev_cyclesInState > timeouts[pev_state];
}

/******* The statemachine dispatcher *******************/
static void pev_runFsm(void)
{
   if (connMgr_getConnectionLevel()<CONNLEVEL_80_TCP_RUNNING)
   {
      /* If we have no TCP to the charger, nothing to do here. Just wait for the link. */
      if (pev_state!=PEV_STATE_NotYetInitialized)
      {
         pev_enterState(PEV_STATE_NotYetInitialized);
         hardwareInterface_setStateB();
         hardwareInterface_setPowerRelayOff();
         pev_isBulbOn = 0;
         pev_cyclesLightBulbDelay = 0;
      }
      return;
   }
   if (connMgr_getConnectionLevel()==CONNLEVEL_80_TCP_RUNNING)
   {
      /* We have a TCP connection. This is the trigger for us. */
      if (pev_state==PEV_STATE_NotYetInitialized) pev_enterState(PEV_STATE_Connected);
   }

   stateFunctions[pev_state](); //call state function

   if (pev_isTooLong())
      pev_enterState(PEV_STATE_SequenceTimeout);
}

/************ public interfaces *****************************************/


/* The cyclic main function of the PEV charging state machine.
   Called each 30ms. */
void pevStateMachine_Mainfunction(void)
{
   // run the state machine:
   pev_cyclesInState += 1; // for timeout handling, count how long we are in a state
   pev_runFsm();
}
