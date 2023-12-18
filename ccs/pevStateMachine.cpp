#include "ccs32_globals.h"
#include "projectExiConnector.h"

/* The Charging State Machine for the car */

#define PEV_STATE_NotYetInitialized 0
#define PEV_STATE_Connecting 1
#define PEV_STATE_Connected 2
#define PEV_STATE_WaitForSupportedApplicationProtocolResponse 3
#define PEV_STATE_WaitForSessionSetupResponse 4
#define PEV_STATE_WaitForServiceDiscoveryResponse 5
#define PEV_STATE_WaitForServicePaymentSelectionResponse 6
#define PEV_STATE_WaitForContractAuthenticationResponse 7
#define PEV_STATE_WaitForChargeParameterDiscoveryResponse 8
#define PEV_STATE_WaitForConnectorLock 9
#define PEV_STATE_WaitForCableCheckResponse 10
#define PEV_STATE_WaitForPreChargeResponse 11
#define PEV_STATE_WaitForContactorsClosed 12
#define PEV_STATE_WaitForPowerDeliveryResponse 13
#define PEV_STATE_WaitForCurrentDemandResponse 14
#define PEV_STATE_WaitForCurrentDownAfterStateB 15
#define PEV_STATE_WaitForWeldingDetectionResponse 16
#define PEV_STATE_WaitForSessionStopResponse 17
//#define PEV_STATE_ChargingFinished 18
#define PEV_STATE_UnrecoverableError 88
#define PEV_STATE_SequenceTimeout 99
#define PEV_STATE_SafeShutDownWaitForChargerShutdown 111
#define PEV_STATE_SafeShutDownWaitForContactorsOpen 222
#define PEV_STATE_End 1000

#define LEN_OF_EVCCID 6 /* The EVCCID is the MAC according to spec. Ioniq uses exactly these 6 byte. */


static const uint8_t exiDemoSupportedApplicationProtocolRequestIoniq[]= {0x80, 0x00, 0xdb, 0xab, 0x93, 0x71, 0xd3, 0x23, 0x4b, 0x71, 0xd1, 0xb9, 0x81, 0x89, 0x91, 0x89, 0xd1, 0x91, 0x81, 0x89, 0x91, 0xd2, 0x6b, 0x9b, 0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00, 0x00, 0x04, 0x00, 0x40 };


static uint16_t pev_cyclesInState;
static uint8_t pev_DelayCycles;
static uint16_t pev_state=PEV_STATE_NotYetInitialized;
static uint8_t pev_isUserStopRequestOnCarSide=0;
static uint8_t pev_isUserStopRequestOnChargerSide=0;
static uint16_t pev_numberOfContractAuthenticationReq;
static uint16_t pev_numberOfChargeParameterDiscoveryReq;
static uint16_t pev_numberOfCableCheckReq;
static uint8_t pev_wasPowerDeliveryRequestedOn;
static uint8_t pev_isBulbOn;
static uint16_t pev_cyclesLightBulbDelay;
static uint16_t EVSEPresentVoltage;
static uint16_t EVSEMinimumVoltage;
static uint8_t numberOfWeldingDetectionRounds;

/***local function prototypes *****************************************/

static uint8_t pev_isTooLong(void);
static void pev_enterState(uint16_t n);

/*** functions ********************************************************/

static int32_t combineValueAndMultiplier(int32_t val, int8_t multiplier)
{
   int32_t x;
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
static void pev_sendChargeParameterDiscoveryReq(void)
{
   struct dinDC_EVChargeParameterType *cp;
   projectExiConnector_prepare_DinExiDocument();
   dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq_isUsed = 1u;
   init_dinChargeParameterDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq);
   dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.EVRequestedEnergyTransferType = dinEVRequestedEnergyTransferType_DC_extended;
   cp = &dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter;
   cp->DC_EVStatus.EVReady = 0;  /* What ever this means. The Ioniq sends 0 here in the ChargeParameterDiscoveryReq message. */
   //cp->DC_EVStatus.EVCabinConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
   //cp->DC_EVStatus.RESSConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
   cp->DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc();
   cp->EVMaximumCurrentLimit.Value = Param::GetInt(Param::maxcur);
   cp->EVMaximumCurrentLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   cp->EVMaximumCurrentLimit.Unit_isUsed = 1;
   cp->EVMaximumCurrentLimit.Unit = dinunitSymbolType_A;

   cp->EVMaximumPowerLimit_isUsed = 1; /* The Ioniq sends 1 here. */
   cp->EVMaximumPowerLimit.Value = Param::GetInt(Param::maxpower) * 10; /* maxpower is kW, then x10 x 100 by Multiplier */
   cp->EVMaximumPowerLimit.Multiplier = 2; /* 10^2 */
   cp->EVMaximumPowerLimit.Unit_isUsed = 1;
   cp->EVMaximumPowerLimit.Unit = dinunitSymbolType_W; /* Watt */

   cp->EVMaximumVoltageLimit.Value = Param::GetInt(Param::maxvtg);
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

   dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter_isUsed = 1;
   encodeAndTransmit();
}

static void pev_sendCableCheckReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   dinDocEnc.V2G_Message.Body.CableCheckReq_isUsed = 1u;
   init_dinCableCheckReqType(&dinDocEnc.V2G_Message.Body.CableCheckReq);
#define st dinDocEnc.V2G_Message.Body.CableCheckReq.DC_EVStatus
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
}

static void pev_sendPreChargeReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   dinDocEnc.V2G_Message.Body.PreChargeReq_isUsed = 1u;
   init_dinPreChargeReqType(&dinDocEnc.V2G_Message.Body.PreChargeReq);
#define st dinDocEnc.V2G_Message.Body.PreChargeReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc(); /* The SOC. Scaling is 1%. */
#undef st
#define tvolt dinDocEnc.V2G_Message.Body.PreChargeReq.EVTargetVoltage
   tvolt.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
   tvolt.Unit = dinunitSymbolType_V;
   tvolt.Unit_isUsed = 1;
   tvolt.Value = hardwareInterface_getAccuVoltage(); /* The precharge target voltage. Scaling is 1V. */
#undef tvolt
#define tcurr dinDocEnc.V2G_Message.Body.PreChargeReq.EVTargetCurrent
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
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq_isUsed = 1u;
   init_dinPowerDeliveryReqType(&dinDocEnc.V2G_Message.Body.PowerDeliveryReq);
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.ReadyToChargeState = isOn; /* 1=ON, 0=OFF */
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter_isUsed = 1;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVReady = 1; /* 1 means true. We are ready. */
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc();
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.ChargingComplete = 0; /* boolean. Charging not finished. */
   /* some "optional" fields seem to be mandatory, at least the Ioniq sends them, and the Compleo charger ignores the message if too short.
      See https://github.com/uhi22/OpenV2Gx/commit/db2c7addb0cae0e16175d666e736efd551f3e14d#diff-333579da65917bc52ef70369b576374d0ee5dbca47d2b1e3bedb6f062decacff
      Let's fill them:
   */
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVCabinConditioning_isUsed = 1;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVCabinConditioning = 0;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSConditioning_isUsed = 1;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.DC_EVStatus.EVRESSConditioning = 0;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.BulkChargingComplete_isUsed  = 1;
   dinDocEnc.V2G_Message.Body.PowerDeliveryReq.DC_EVPowerDeliveryParameter.BulkChargingComplete = 0;
   encodeAndTransmit();
}

static void pev_sendCurrentDemandReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   dinDocEnc.V2G_Message.Body.CurrentDemandReq_isUsed = 1u;
   init_dinCurrentDemandReqType(&dinDocEnc.V2G_Message.Body.CurrentDemandReq);
   // DC_EVStatus
#define st dinDocEnc.V2G_Message.Body.CurrentDemandReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc();
#undef st
   // EVTargetVoltage
#define tvolt dinDocEnc.V2G_Message.Body.CurrentDemandReq.EVTargetVoltage
   tvolt.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   tvolt.Unit = dinunitSymbolType_V;
   tvolt.Unit_isUsed = 1;
   tvolt.Value = hardwareInterface_getChargingTargetVoltage(); /* The charging target. Scaling is 1V. */
#undef tvolt
   // EVTargetCurrent
#define tcurr dinDocEnc.V2G_Message.Body.CurrentDemandReq.EVTargetCurrent
   tcurr.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   tcurr.Unit = dinunitSymbolType_A;
   tcurr.Unit_isUsed = 1;
   tcurr.Value = pev_wasPowerDeliveryRequestedOn ? hardwareInterface_getChargingTargetCurrent() : 0; /* The charging target current. Scaling is 1A. */
#undef tcurr
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.ChargingComplete = 0; /* boolean. Todo: Do we need to take this from command line? Or is it fine
    that the PEV just sends a PowerDeliveryReq with STOP, if it decides to stop the charging? */
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete_isUsed = 1u;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.BulkChargingComplete = 0u; /* not complete */
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC_isUsed = 1u;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit = dinunitSymbolType_s;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Unit_isUsed = 1;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToFullSoC.Value = 1200; /* seconds */

   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC_isUsed = 1u;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Multiplier = 0;  /* -3 to 3. The exponent for base of 10. */
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit = dinunitSymbolType_s;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Unit_isUsed = 1;
   dinDocEnc.V2G_Message.Body.CurrentDemandReq.RemainingTimeToBulkSoC.Value = 600; /* seconds */
   encodeAndTransmit();
}

static void pev_sendWeldingDetectionReq(void)
{
   projectExiConnector_prepare_DinExiDocument();
   dinDocEnc.V2G_Message.Body.WeldingDetectionReq_isUsed = 1u;
   init_dinWeldingDetectionReqType(&dinDocEnc.V2G_Message.Body.WeldingDetectionReq);
#define st dinDocEnc.V2G_Message.Body.WeldingDetectionReq.DC_EVStatus
   st.EVReady = 1; /* 1 means true. We are ready. */
   st.EVErrorCode = dinDC_EVErrorCodeType_NO_ERROR;
   st.EVRESSSOC = hardwareInterface_getSoc();
#undef st
   encodeAndTransmit();
}

/**** State functions ***************/

static void stateFunctionConnected(void)
{
   // We have a freshly established TCP channel. We start the V2GTP/EXI communication now.
   // We just use the initial request message from the Ioniq. It contains one entry: DIN.
   addToTrace(MOD_PEV, "Checkpoint400: Sending the initial SupportedApplicationProtocolReq");
   setCheckpoint(400);
   addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));
   hardwareInterface_resetSimulation();
   pev_enterState(PEV_STATE_WaitForSupportedApplicationProtocolResponse);
}

static void stateFunctionWaitForSupportedApplicationProtocolResponse(void)
{
   uint8_t i;
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
         publishStatus("Schema negotiated", "");
         addToTrace(MOD_PEV, "Checkpoint403: Schema negotiated. And Checkpoint500: Will send SessionSetupReq");
         setCheckpoint(500);
         projectExiConnector_prepare_DinExiDocument();
         dinDocEnc.V2G_Message.Body.SessionSetupReq_isUsed = 1u;
         init_dinSessionSetupReqType(&dinDocEnc.V2G_Message.Body.SessionSetupReq);
         /* In the session setup request, the session ID zero means: create a new session.
             The format (len 8, all zero) is taken from the original Ioniq behavior. */
         dinDocEnc.V2G_Message.Header.SessionID.bytes[0] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[1] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[2] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[3] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[4] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[5] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[6] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytes[7] = 0;
         dinDocEnc.V2G_Message.Header.SessionID.bytesLen = 8;
         /* Takeover from https://github.com/uhi22/OpenV2Gx/commit/fc46c3ca802f08c57120a308f69fb4d1ce14f6b6 */
         /* The EVCCID. In the ISO they write, that this shall be the EVCC MAC. But the DIN
            reserves 8 bytes (dinSessionSetupReqType_EVCCID_BYTES_SIZE is 8). This does not match.
            The Ioniq (DIN) sets the bytesLen=6 and fills the 6 bytes with its own MAC. Let's assume this
            is the best way. */
         for (i=0; i<LEN_OF_EVCCID; i++)
         {
            dinDocEnc.V2G_Message.Body.SessionSetupReq.EVCCID.bytes[i] = myMAC[i];
         }
         dinDocEnc.V2G_Message.Body.SessionSetupReq.EVCCID.bytesLen = LEN_OF_EVCCID;
         encodeAndTransmit();
         pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
      }
   }
   if (pev_isTooLong())
   {
      ErrorMessage::Post(ERR_PLCTIMEOUT);
      pev_enterState(PEV_STATE_SequenceTimeout);
   }
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
      if (dinDocDec.V2G_Message.Body.SessionSetupRes_isUsed)
      {
         memcpy(sessionId, dinDocDec.V2G_Message.Header.SessionID.bytes, SESSIONID_LEN);
         sessionIdLen = dinDocDec.V2G_Message.Header.SessionID.bytesLen; /* store the received SessionID, we will need it later. */
         addToTrace(MOD_PEV, "Checkpoint506: The Evse decided for SessionId", sessionId, sessionIdLen);
         setCheckpoint(506);
         publishStatus("Session established", "");
         addToTrace(MOD_PEV, "Will send ServiceDiscoveryReq");
         projectExiConnector_prepare_DinExiDocument();
         dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq_isUsed = 1u;
         init_dinServiceDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq);
         setCheckpoint(510);
         encodeAndTransmit();
         pev_enterState(PEV_STATE_WaitForServiceDiscoveryResponse);
      }
   }
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.ServiceDiscoveryRes_isUsed)
      {
         publishStatus("ServDisc done", "");
         addToTrace(MOD_PEV, "Will send ServicePaymentSelectionReq");
         projectExiConnector_prepare_DinExiDocument();
         dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq_isUsed = 1u;
         init_dinServicePaymentSelectionReqType(&dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq);
         /* the mandatory fields in ISO are SelectedPaymentOption and SelectedServiceList. Same in DIN. */
         dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedPaymentOption = dinpaymentOptionType_ExternalPayment; /* not paying per car */
         dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.array[0].ServiceID = 1; /* todo: what ever this means. The Ioniq uses 1. */
         dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.arrayLen = 1; /* just one element in the array */
         setCheckpoint(520);
         encodeAndTransmit();
         pev_enterState(PEV_STATE_WaitForServicePaymentSelectionResponse);
      }
   }
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.ServicePaymentSelectionRes_isUsed)
      {
         publishStatus("ServPaySel done", "");
         addToTrace(MOD_PEV, "Checkpoint530: Will send ContractAuthenticationReq");
         setCheckpoint(530);
         projectExiConnector_prepare_DinExiDocument();
         dinDocEnc.V2G_Message.Body.ContractAuthenticationReq_isUsed = 1u;
         init_dinContractAuthenticationReqType(&dinDocEnc.V2G_Message.Body.ContractAuthenticationReq);
         /* no other fields are manatory */
         encodeAndTransmit();
         pev_numberOfContractAuthenticationReq = 1; // This is the first request.
         pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
      }
   }
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes_isUsed)
      {
         // In normal case, we can have two results here: either the Authentication is needed (the user
         // needs to authorize by RFID card or app, or something like this.
         // Or, the authorization is finished. This is shown by EVSEProcessing=Finished.
         if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes.EVSEProcessing == dinEVSEProcessingType_Finished)
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes_isUsed)
      {
         // We can have two cases here:
         // (A) The charger needs more time to show the charge parameters.
         // (B) The charger finished to tell the charge parameters.
         if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.EVSEProcessing == dinEVSEProcessingType_Finished)
         {
            publishStatus("ChargeParams discovered", "");
            addToTrace(MOD_PEV, "Checkpoint550: ChargeParams are discovered.. Will change to state C.");
            uint16_t evseMaxVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMaximumVoltageLimit.Value,
                                      dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMaximumVoltageLimit.Multiplier);
            uint16_t evseMaxCurrent = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMaximumCurrentLimit.Value,
                                      dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMaximumCurrentLimit.Multiplier);
            EVSEMinimumVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMinimumVoltageLimit.Value,
                                      dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.DC_EVSEChargeParameter.EVSEMinimumVoltageLimit.Multiplier);

            Param::SetInt(Param::evsemaxvtg, evseMaxVoltage);
            Param::SetInt(Param::evsemaxcur, evseMaxCurrent);

            setCheckpoint(550);
            // pull the CP line to state C here:
            hardwareInterface_setStateC();
            addToTrace(MOD_PEV, "Checkpoint555: Locking the connector.");
            hardwareInterface_triggerConnectorLocking();
            //If we are not ready for charging, don't go past this state -> will time out
            if (hardwareInterface_stopCharging())
            {
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
   {
      ErrorMessage::Post(ERR_LOCKTIMEOUT);
      pev_enterState(PEV_STATE_SequenceTimeout);
   }
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
      if (dinDocDec.V2G_Message.Body.CableCheckRes_isUsed)
      {
         rc = dinDocDec.V2G_Message.Body.CableCheckRes.ResponseCode;
         proc = dinDocDec.V2G_Message.Body.CableCheckRes.EVSEProcessing;
         Param::SetInt(Param::evsevtg, 0);
         pev_isUserStopRequestOnCarSide = 0;
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
#if 0 /* todo: use config item to decide whether we have inlet voltage measurement or not */
               publishStatus("CbleChck ongoing", String(hardwareInterface_getInletVoltage()) + "V");
#else
               /* no inlet voltage measurement available, just show status */
               publishStatus("CbleChck ongoing", "");
#endif
               addToTrace(MOD_PEV, "Will again send CableCheckReq");
               pev_sendCableCheckReq();
               // stay in the same state
               pev_enterState(PEV_STATE_WaitForCableCheckResponse);
            }
         }
      }
   }
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.PreChargeRes_isUsed)
      {
         addToTrace(MOD_PEV, "PreCharge aknowledge received.");
         EVSEPresentVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Value,
                              dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Multiplier);
         Param::SetInt(Param::evsevtg, EVSEPresentVoltage);

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
   {
      ErrorMessage::Post(ERR_PRECTIMEOUT);
      pev_enterState(PEV_STATE_SequenceTimeout);
   }
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.PowerDeliveryRes_isUsed)
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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

      showAsHex(tcp_rxdata, tcp_rxdataLen, "");

      routeDecoderInputData();
      //printf("[%d] step1 %d\r\n", rtc_get_ms(), tcp_rxdataLen);
      projectExiConnector_decode_DinExiDocument();

      //printf("[%d] step2 %d %d\r\n", rtc_get_ms(), g_errn, global_streamDecPos);

      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (dinDocDec.V2G_Message.Body.CurrentDemandRes_isUsed)
      {
         /* as long as the accu is not full and no stop-demand from the user, we continue charging */
         pev_isUserStopRequestOnChargerSide=0;
         if (dinDocDec.V2G_Message.Body.CurrentDemandRes.DC_EVSEStatus.EVSEStatusCode == dinDC_EVSEStatusCodeType_EVSE_Shutdown)
         {
            /* https://github.com/uhi22/pyPLC#example-flow, checkpoint 790: If the user stops the
               charging session on the charger, we get a CurrentDemandResponse with
               DC_EVSEStatus.EVSEStatusCode = 2 "EVSE_Shutdown" (observed on Compleo. To be tested
               on other chargers. */
            addToTrace(MOD_PEV, "Checkpoint790: Charging is terminated from charger side.");
            setCheckpoint(790);
            pev_isUserStopRequestOnChargerSide = 1;
         }
         if (dinDocDec.V2G_Message.Body.CurrentDemandRes.DC_EVSEStatus.EVSEStatusCode == dinDC_EVSEStatusCodeType_EVSE_EmergencyShutdown)
         {
            /* If the charger reports an emergency, we stop the charging. */
            addToTrace(MOD_PEV, "Charger reported EmergencyShutdown.");
            pev_wasPowerDeliveryRequestedOn=0;
            setCheckpoint(800);
            pev_sendPowerDeliveryReq(0);
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
         }

         /* If the pushbutton is pressed longer than 0.5s, we interpret this as charge stop request. If pressed, remember */
         pev_isUserStopRequestOnCarSide |= pushbutton_tButtonPressTime>(PUSHBUTTON_CYCLES_PER_SECOND/2);
         EVSEPresentVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentVoltage.Value,
                              dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentVoltage.Multiplier);
         uint16_t evsePresentCurrent = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentCurrent.Value,
                                       dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentCurrent.Multiplier);


         if (hardwareInterface_getIsAccuFull() || pev_isUserStopRequestOnCarSide || pev_isUserStopRequestOnChargerSide)
         {
            if (hardwareInterface_getIsAccuFull())
            {
               publishStatus("Accu full", "");
               addToTrace(MOD_PEV, "Accu is full. Waiting for current to drop below 1A");
            }
            else if (pev_isUserStopRequestOnCarSide)
            {
               publishStatus("User req stop on car side", "");
               addToTrace(MOD_PEV, "User requested stop on car side. Waiting for current to drop below 1A");
            }
            else
            {
               publishStatus("User req stop on charger side", "");
               addToTrace(MOD_PEV, "User requested stop on charger side. Waiting for current to drop below 1A");
            }
            pev_wasPowerDeliveryRequestedOn=0;
            setCheckpoint(800);

            if (evsePresentCurrent < 1)
            {
               addToTrace(MOD_PEV, "Current dropped to 0. Sending PowerDeliveryReq Stop.");
               pev_sendPowerDeliveryReq(0);
               pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
            }
            else
            {
               pev_sendCurrentDemandReq();
               pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
            }

         }
         else
         {
            /* continue charging loop */
            hardwareInterface_simulateCharging();
            //publishStatus("Charging", String(u) + "V", String(hardwareInterface_getSoc()) + "%");
            Param::SetInt(Param::evsevtg, EVSEPresentVoltage);
            Param::SetInt(Param::evsecur, evsePresentCurrent);
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
   }
}

#define MAX_VOLTAGE_TO_FINISH_WELDING_DETECTION 40 /* 40V is considered to be sufficiently low to not harm. The Ioniq already finishes at 65V. */
#define MAX_NUMBER_OF_WELDING_DETECTION_ROUNDS 10 /* The process time is specified with 1.5s. Ten loops should be fine. */

static void stateFunctionWaitForWeldingDetectionResponse(void)
{
   if (tcp_rxdataLen>V2GTP_HEADER_SIZE)
   {
      addToTrace(MOD_PEV, "In state WaitForWeldingDetectionRes");
      routeDecoderInputData();
      projectExiConnector_decode_DinExiDocument();
      tcp_rxdataLen = 0; /* mark the input data as "consumed" */
      if (dinDocDec.V2G_Message.Body.WeldingDetectionRes_isUsed)
      {
        /* The charger measured the voltage on the cable, and gives us the value. In the first
           round will show a quite high voltage, because the contactors are just opening. We
           need to repeat the requests, until the voltage is at a non-dangerous level. */
         EVSEPresentVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.WeldingDetectionRes.EVSEPresentVoltage.Value,
                              dinDocDec.V2G_Message.Body.WeldingDetectionRes.EVSEPresentVoltage.Multiplier);
         Param::SetInt(Param::evsevtg, EVSEPresentVoltage);
         if (Param::GetInt(Param::logging) & MOD_PEV) {
             printf("EVSEPresentVoltage %dV\r\n", EVSEPresentVoltage);
         }
         if (EVSEPresentVoltage<MAX_VOLTAGE_TO_FINISH_WELDING_DETECTION) {
            /* voltage is low, weldingDetection finished successfully. */
            publishStatus("WeldingDet done", "");
            addToTrace(MOD_PEV, "WeldingDetection successfully finished. Sending SessionStopReq");
            projectExiConnector_prepare_DinExiDocument();
            dinDocEnc.V2G_Message.Body.SessionStopReq_isUsed = 1u;
            init_dinSessionStopType(&dinDocEnc.V2G_Message.Body.SessionStopReq);
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
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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
      if (dinDocDec.V2G_Message.Body.SessionStopRes_isUsed)
      {
         // req -508
         // Todo: close the TCP connection here.
         publishStatus("Stopped normally", "");
         addToTrace(MOD_PEV, "Charging is finished");
         pev_enterState(PEV_STATE_End);
      }
   }
   if (pev_isTooLong())
   {
      pev_enterState(PEV_STATE_SequenceTimeout);
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

static void pev_enterState(uint16_t n)
{
   //printf("[%d] [PEV] from %d entering %d\r\n", rtc_get_ms(), pev_state, n);
   pev_state = n;
   pev_cyclesInState = 0;
   Param::SetInt(Param::opmode, MIN(n, 18));
}

static uint8_t pev_isTooLong(void)
{
   uint16_t limit;
   /* The timeout handling function. */
   limit = 66; /* number of call cycles until timeout. Default 66 cycles with 30ms, means approx. 2 seconds.
        This 2s is the specified timeout time for many messages, fitting to the
        performance time of 1.5s. Exceptions see below. */
   if (pev_state==PEV_STATE_WaitForChargeParameterDiscoveryResponse)
   {
      limit = 5*33; /* On some charger models, the chargeParameterDiscovery needs more than a second. Wait at least 5s. */
   }
   if (pev_state==PEV_STATE_WaitForCableCheckResponse)
   {
      limit = 30*33; // CableCheck may need some time. Wait at least 30s.
   }
   if (pev_state==PEV_STATE_WaitForPreChargeResponse)
   {
      limit = 30*33; // PreCharge may need some time. Wait at least 30s.
   }
   if (pev_state==PEV_STATE_WaitForPowerDeliveryResponse)
   {
      limit = 6*33; /* PowerDelivery may need some time. Wait at least 6s. On Compleo charger, observed more than 1s until response.
                     specified performance time is 4.5s (ISO) */
   }
   if (pev_state==PEV_STATE_WaitForCurrentDemandResponse)
   {
      limit = 5*33;  /* Test with 5s timeout. Just experimental.
                      The specified performance time is 25ms (ISO), the specified timeout 250ms. */
   }
   return (pev_cyclesInState > limit);
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
   switch (pev_state)
   {
   case PEV_STATE_Connected:
      stateFunctionConnected();
      break;
   case PEV_STATE_WaitForSupportedApplicationProtocolResponse:
      stateFunctionWaitForSupportedApplicationProtocolResponse();
      break;
   case PEV_STATE_WaitForSessionSetupResponse:
      stateFunctionWaitForSessionSetupResponse();
      break;
   case PEV_STATE_WaitForServiceDiscoveryResponse:
      stateFunctionWaitForServiceDiscoveryResponse();
      break;
   case PEV_STATE_WaitForServicePaymentSelectionResponse:
      stateFunctionWaitForServicePaymentSelectionResponse();
      break;
   case PEV_STATE_WaitForContractAuthenticationResponse:
      stateFunctionWaitForContractAuthenticationResponse();
      break;
   case PEV_STATE_WaitForChargeParameterDiscoveryResponse:
      stateFunctionWaitForChargeParameterDiscoveryResponse();
      break;
   case PEV_STATE_WaitForConnectorLock:
      stateFunctionWaitForConnectorLock();
      break;
   case PEV_STATE_WaitForCableCheckResponse:
      stateFunctionWaitForCableCheckResponse();
      break;
   case PEV_STATE_WaitForPreChargeResponse:
      stateFunctionWaitForPreChargeResponse();
      break;
   case PEV_STATE_WaitForContactorsClosed:
      stateFunctionWaitForContactorsClosed();
      break;
   case PEV_STATE_WaitForPowerDeliveryResponse:
      stateFunctionWaitForPowerDeliveryResponse();
      break;
   case PEV_STATE_WaitForCurrentDemandResponse:
      stateFunctionWaitForCurrentDemandResponse();
      break;
   case PEV_STATE_WaitForCurrentDownAfterStateB:
      stateFunctionWaitForCurrentDownAfterStateB();
      break;
   case PEV_STATE_WaitForWeldingDetectionResponse:
      stateFunctionWaitForWeldingDetectionResponse();
      break;
   case PEV_STATE_WaitForSessionStopResponse:
      stateFunctionWaitForSessionStopResponse();
      break;
   case PEV_STATE_UnrecoverableError:
      stateFunctionUnrecoverableError();
      break;
   case PEV_STATE_SequenceTimeout:
      stateFunctionSequenceTimeout();
      break;
   case PEV_STATE_SafeShutDownWaitForChargerShutdown:
      stateFunctionSafeShutDownWaitForChargerShutdown();
      break;
   case PEV_STATE_SafeShutDownWaitForContactorsOpen:
      stateFunctionSafeShutDownWaitForContactorsOpen();
      break;
   case PEV_STATE_End:
      stateFunctionEnd();
      break;
   }
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
