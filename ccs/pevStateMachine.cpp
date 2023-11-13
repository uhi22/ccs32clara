
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
#define PEV_STATE_WaitForWeldingDetectionResponse 15
#define PEV_STATE_WaitForSessionStopResponse 16
#define PEV_STATE_ChargingFinished 17
#define PEV_STATE_UnrecoverableError 88
#define PEV_STATE_SequenceTimeout 99
#define PEV_STATE_SafeShutDownWaitForChargerShutdown 111
#define PEV_STATE_SafeShutDownWaitForContactorsOpen 222
#define PEV_STATE_End 1000

#define LEN_OF_EVCCID 6 /* The EVCCID is the MAC according to spec. Ioniq uses exactly these 6 byte. */


const uint8_t exiDemoSupportedApplicationProtocolRequestIoniq[]={0x80, 0x00, 0xdb, 0xab, 0x93, 0x71, 0xd3, 0x23, 0x4b, 0x71, 0xd1, 0xb9, 0x81, 0x89, 0x91, 0x89, 0xd1, 0x91, 0x81, 0x89, 0x91, 0xd2, 0x6b, 0x9b, 0x3a, 0x23, 0x2b, 0x30, 0x02, 0x00, 0x00, 0x04, 0x00, 0x40 };


uint16_t pev_cyclesInState;
uint8_t pev_DelayCycles;
uint16_t pev_state=PEV_STATE_NotYetInitialized;
uint8_t pev_isUserStopRequest=0;
uint16_t pev_numberOfContractAuthenticationReq;
uint16_t pev_numberOfChargeParameterDiscoveryReq;
uint16_t pev_numberOfCableCheckReq;
uint8_t pev_wasPowerDeliveryRequestedOn;
uint8_t pev_isBulbOn;
uint16_t pev_cyclesLightBulbDelay;
uint16_t EVSEPresentVoltage;

/***local function prototypes *****************************************/

uint8_t pev_isTooLong(void);
void pev_enterState(uint16_t n);

/*** functions ********************************************************/

int32_t combineValueAndMultiplier(int32_t val, int8_t multiplier) {
  int32_t x;
  x = val;
  while (multiplier>0) { x=x*10; multiplier--; }
  while (multiplier<0) { x=x/10; multiplier++; }
  return x;
}

void addV2GTPHeaderAndTransmit(const uint8_t *exiBuffer, uint8_t exiBufferLen) {
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
  if (exiBufferLen+8<TCP_PAYLOAD_LEN) {
      memcpy(&tcpPayload[8], exiBuffer, exiBufferLen);
      tcpPayloadLen = 8 + exiBufferLen; /* 8 byte V2GTP header, plus the EXI data */
      //log_v("Step3 %d", tcpPayloadLen);
      showAsHex(tcpPayload, tcpPayloadLen, "tcpPayload");
      tcp_transmit();
  } else {
      addToTrace("Error: EXI does not fit into tcpPayload.");
  }
}

void encodeAndTransmit(void) {
  /* calls the EXI encoder, adds the V2GTP header and sends the result to ethernet */
  //addToTrace("before: g_errn=" + String(g_errn));
  //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
  global_streamEncPos = 0;
  projectExiConnector_encode_DinExiDocument();
  //addToTrace("after: g_errn=" + String(g_errn));
  //addToTrace("global_streamEncPos=" + String(global_streamEncPos));
  addV2GTPHeaderAndTransmit(global_streamEnc.data, global_streamEncPos);
}

void routeDecoderInputData(void) {
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
void pev_sendChargeParameterDiscoveryReq(void) {
    struct dinDC_EVChargeParameterType *cp;
    projectExiConnector_prepare_DinExiDocument();
    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq_isUsed = 1u;
    init_dinChargeParameterDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq);
    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.EVRequestedEnergyTransferType = dinEVRequestedEnergyTransferType_DC_extended;
    cp = &dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter;
    cp->DC_EVStatus.EVReady = 0;  /* What ever this means. The Ioniq sends 0 here in the ChargeParameterDiscoveryReq message. */
    //cp->DC_EVStatus.EVCabinConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
    //cp->DC_EVStatus.RESSConditioning_isUsed /* The Ioniq sends this with 1, but let's assume it is not mandatory. */
    cp->DC_EVStatus.EVRESSSOC = hardwareInterface_getSoc(); /* Todo: Take the SOC from the BMS. Scaling is 1%. */
    cp->EVMaximumCurrentLimit.Value = 100;
    cp->EVMaximumCurrentLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
    cp->EVMaximumCurrentLimit.Unit_isUsed = 1;
    cp->EVMaximumCurrentLimit.Unit = dinunitSymbolType_A;

    cp->EVMaximumPowerLimit_isUsed = 1; /* The Ioniq sends 1 here. */
    cp->EVMaximumPowerLimit.Value = 9800; /* Ioniq: 9800 */
    cp->EVMaximumPowerLimit.Multiplier = 1; /* 10^1 */
    cp->EVMaximumPowerLimit.Unit_isUsed = 1;
    cp->EVMaximumPowerLimit.Unit = dinunitSymbolType_W; /* Watt */

    cp->EVMaximumVoltageLimit.Value = 398;
    cp->EVMaximumVoltageLimit.Multiplier = 0; /* -3 to 3. The exponent for base of 10. */
    cp->EVMaximumVoltageLimit.Unit_isUsed = 1;
    cp->EVMaximumVoltageLimit.Unit = dinunitSymbolType_V;

    cp->EVEnergyCapacity_isUsed = 1;
    cp->EVEnergyCapacity.Value = 28000; /* 28kWh from Ioniq */
    cp->EVEnergyCapacity.Multiplier = 0;
    cp->EVEnergyCapacity.Unit_isUsed = 1;
    cp->EVEnergyCapacity.Unit = dinunitSymbolType_Wh; /* from Ioniq */

    cp->EVEnergyRequest_isUsed = 1;
    cp->EVEnergyRequest.Value = 20000; /* just invented 20kWh */
    cp->EVEnergyRequest.Multiplier = 0;
    cp->EVEnergyRequest.Unit_isUsed = 1;
    cp->EVEnergyRequest.Unit = dinunitSymbolType_Wh; /* 9 from Ioniq */

    cp->FullSOC_isUsed = 1;
    cp->FullSOC = 100;
    cp->BulkSOC_isUsed = 1;
    cp->BulkSOC = 80;

    dinDocEnc.V2G_Message.Body.ChargeParameterDiscoveryReq.DC_EVChargeParameter_isUsed = 1;
    encodeAndTransmit();
}

void pev_sendCableCheckReq(void) {
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

void pev_sendPreChargeReq(void) {
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

void pev_sendPowerDeliveryReq(uint8_t isOn) {
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

void pev_sendCurrentDemandReq(void) {
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
    tcurr.Value = hardwareInterface_getChargingTargetCurrent(); /* The charging target current. Scaling is 1A. */
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

void pev_sendWeldingDetectionReq(void) {
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

void stateFunctionConnected(void) {
  // We have a freshly established TCP channel. We start the V2GTP/EXI communication now.
  // We just use the initial request message from the Ioniq. It contains one entry: DIN.
  addToTrace("Checkpoint400: Sending the initial SupportedApplicationProtocolReq");
  checkpointNumber = 400;
  addV2GTPHeaderAndTransmit(exiDemoSupportedApplicationProtocolRequestIoniq, sizeof(exiDemoSupportedApplicationProtocolRequestIoniq));
  hardwareInterface_resetSimulation();
  pev_enterState(PEV_STATE_WaitForSupportedApplicationProtocolResponse);
}

void stateFunctionWaitForSupportedApplicationProtocolResponse(void) {
  uint8_t i;
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForSupportedApplicationProtocolResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_appHandExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (aphsDoc.supportedAppProtocolRes_isUsed) {
        /* it is the correct response */
        addToTrace("supportedAppProtocolRes");
        printf("ResponseCode %d, SchemaID_isUsed %d, SchemaID %d\r\n",
                      aphsDoc.supportedAppProtocolRes.ResponseCode,
                      aphsDoc.supportedAppProtocolRes.SchemaID_isUsed,
                      aphsDoc.supportedAppProtocolRes.SchemaID);
        publishStatus("Schema negotiated", "");
        addToTrace("Checkpoint403: Schema negotiated. And Checkpoint500: Will send SessionSetupReq");
        checkpointNumber = 500;
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
        for (i=0; i<LEN_OF_EVCCID; i++) {
          dinDocEnc.V2G_Message.Body.SessionSetupReq.EVCCID.bytes[i] = myMAC[i];
        }
        dinDocEnc.V2G_Message.Body.SessionSetupReq.EVCCID.bytesLen = LEN_OF_EVCCID;
        encodeAndTransmit();
        pev_enterState(PEV_STATE_WaitForSessionSetupResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForSessionSetupResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForSessionSetupResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    //addToTrace("after decoding: g_errn=" + String(g_errn));
    //addToTrace("global_streamDecPos=" + String(global_streamDecPos));
    if (dinDocDec.V2G_Message.Body.SessionSetupRes_isUsed) {
      memcpy(sessionId, dinDocDec.V2G_Message.Header.SessionID.bytes, SESSIONID_LEN);
      sessionIdLen = dinDocDec.V2G_Message.Header.SessionID.bytesLen; /* store the received SessionID, we will need it later. */
      addToTrace("Checkpoint506: The Evse decided for SessionId");
      checkpointNumber = 506;
      showAsHex(sessionId, sessionIdLen, "");
      publishStatus("Session established", "");
      addToTrace("Will send ServiceDiscoveryReq");
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq_isUsed = 1u;
      init_dinServiceDiscoveryReqType(&dinDocEnc.V2G_Message.Body.ServiceDiscoveryReq);
      checkpointNumber = 510;
      encodeAndTransmit();
      pev_enterState(PEV_STATE_WaitForServiceDiscoveryResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForServiceDiscoveryResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForServiceDiscoveryResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ServiceDiscoveryRes_isUsed) {
      publishStatus("ServDisc done", "");
      addToTrace("Will send ServicePaymentSelectionReq");
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq_isUsed = 1u;
      init_dinServicePaymentSelectionReqType(&dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq);
      /* the mandatory fields in ISO are SelectedPaymentOption and SelectedServiceList. Same in DIN. */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedPaymentOption = dinpaymentOptionType_ExternalPayment; /* not paying per car */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.array[0].ServiceID = 1; /* todo: what ever this means. The Ioniq uses 1. */
      dinDocEnc.V2G_Message.Body.ServicePaymentSelectionReq.SelectedServiceList.SelectedService.arrayLen = 1; /* just one element in the array */
      checkpointNumber = 520;
      encodeAndTransmit();
      pev_enterState(PEV_STATE_WaitForServicePaymentSelectionResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForServicePaymentSelectionResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForServicePaymentSelectionResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ServicePaymentSelectionRes_isUsed) {
      publishStatus("ServPaySel done", "");
      addToTrace("Checkpoint530: Will send ContractAuthenticationReq");
      checkpointNumber = 530;
      projectExiConnector_prepare_DinExiDocument();
      dinDocEnc.V2G_Message.Body.ContractAuthenticationReq_isUsed = 1u;
      init_dinContractAuthenticationReqType(&dinDocEnc.V2G_Message.Body.ContractAuthenticationReq);
      /* no other fields are manatory */
      encodeAndTransmit();
      pev_numberOfContractAuthenticationReq = 1; // This is the first request.
      pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForContractAuthenticationResponse(void) {
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForContractAuthenticationResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes_isUsed) {
        // In normal case, we can have two results here: either the Authentication is needed (the user
        // needs to authorize by RFID card or app, or something like this.
        // Or, the authorization is finished. This is shown by EVSEProcessing=Finished.
        if (dinDocDec.V2G_Message.Body.ContractAuthenticationRes.EVSEProcessing == dinEVSEProcessingType_Finished) {
            publishStatus("Auth finished", "");
            addToTrace("Checkpoint538 and 540: Auth is Finished. Will send ChargeParameterDiscoveryReq");
            checkpointNumber = 540;
            pev_sendChargeParameterDiscoveryReq();
            pev_numberOfChargeParameterDiscoveryReq = 1; // first message
            pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse);
        } else {
            // Not (yet) finished.
            if (pev_numberOfContractAuthenticationReq>=120) { // approx 120 seconds, maybe the user searches two minutes for his RFID card...
                addToTrace("Authentication lasted too long. Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {
                // Try again.
                pev_numberOfContractAuthenticationReq += 1; // count the number of tries.
                publishStatus("Waiting f Auth", "");
                //addToTrace("Not (yet) finished. Will again send ContractAuthenticationReq #" + String(pev_numberOfContractAuthenticationReq));
                addToTrace("Not (yet) finished. Will again send ContractAuthenticationReq");
                encodeAndTransmit();
                // We just stay in the same state, until the timeout elapses.
                pev_enterState(PEV_STATE_WaitForContractAuthenticationResponse);
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForChargeParameterDiscoveryResponse(void) {
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForChargeParameterDiscoveryResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes_isUsed) {
        // We can have two cases here:
        // (A) The charger needs more time to show the charge parameters.
        // (B) The charger finished to tell the charge parameters.
        if (dinDocDec.V2G_Message.Body.ChargeParameterDiscoveryRes.EVSEProcessing == dinEVSEProcessingType_Finished) {
            publishStatus("ChargeParams discovered", "");
            addToTrace("Checkpoint550: ChargeParams are discovered.. Will change to state C.");
            checkpointNumber = 550;
            // pull the CP line to state C here:
            hardwareInterface_setStateC();
            addToTrace("Checkpoint555: Locking the connector.");
            hardwareInterface_triggerConnectorLocking();
            pev_enterState(PEV_STATE_WaitForConnectorLock);
        } else {
            // Not (yet) finished.
            if (pev_numberOfChargeParameterDiscoveryReq>=60) { /* approx 60 seconds, should be sufficient for the charger to find its parameters.
                ... The ISO allows up to 55s reaction time and 60s timeout for "ongoing". Taken over from
                    https://github.com/uhi22/pyPLC/commit/01c7c069fd4e7b500aba544ae4cfce6774f7344a */
                //addToTrace("ChargeParameterDiscovery lasted too long. " + String(pev_numberOfChargeParameterDiscoveryReq) + " Giving up.");
                addToTrace("ChargeParameterDiscovery lasted too long. Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {
                // Try again.
                pev_numberOfChargeParameterDiscoveryReq += 1; // count the number of tries.
                publishStatus("disc ChargeParams", "");
                //addToTrace("Not (yet) finished. Will again send ChargeParameterDiscoveryReq #" + String(pev_numberOfChargeParameterDiscoveryReq));
                addToTrace("Not (yet) finished. Will again send ChargeParameterDiscoveryReq");
                pev_sendChargeParameterDiscoveryReq();
                // we stay in the same state
                pev_enterState(PEV_STATE_WaitForChargeParameterDiscoveryResponse);
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForConnectorLock(void) {
  if (hardwareInterface_isConnectorLocked()) {
    addToTrace("Checkpoint560: Connector Lock confirmed. Will send CableCheckReq.");
    checkpointNumber = 560;
    pev_sendCableCheckReq();
    pev_numberOfCableCheckReq = 1; // This is the first request.
    pev_enterState(PEV_STATE_WaitForCableCheckResponse);
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForCableCheckResponse(void) {
  uint8_t rc, proc;
  if (pev_cyclesInState<30) { // The first second in the state just do nothing.
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForCableCheckResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.CableCheckRes_isUsed) {
        rc = dinDocDec.V2G_Message.Body.CableCheckRes.ResponseCode;
        proc = dinDocDec.V2G_Message.Body.CableCheckRes.EVSEProcessing;
        //addToTrace("The CableCheck result is " + String(rc) + " " + String(proc));
        // We have two cases here:
        // 1) The charger says "cable check is finished and cable ok", by setting ResponseCode=OK and EVSEProcessing=Finished.
        // 2) Else: The charger says "need more time or cable not ok". In this case, we just run into timeout and start from the beginning.
        if ((proc==dinEVSEProcessingType_Finished) && (rc==dinresponseCodeType_OK)) {
            publishStatus("CbleChck done", "");
            addToTrace("The EVSE says that the CableCheck is finished and ok.");
            addToTrace("Will send PreChargeReq");
            checkpointNumber = 570;
            pev_sendPreChargeReq();
            connMgr_ApplOk(31); /* PreChargeResponse may need longer. Inform the connection manager to be patient.
                                (This is a takeover from https://github.com/uhi22/pyPLC/commit/08af8306c60d57c4c33221a0dbb25919371197f9 ) */
            pev_enterState(PEV_STATE_WaitForPreChargeResponse);
        } else {
            if (pev_numberOfCableCheckReq>60) { /* approx 60s should be sufficient for cable check. The ISO allows up to 55s reaction time and 60s timeout for "ongoing". Taken over from https://github.com/uhi22/pyPLC/commit/01c7c069fd4e7b500aba544ae4cfce6774f7344a */
                //addToTrace("CableCheck lasted too long. " + String(pev_numberOfCableCheckReq) + " Giving up.");
                addToTrace("CableCheck lasted too long. Giving up.");
                pev_enterState(PEV_STATE_SequenceTimeout);
            } else {
                // cable check not yet finished or finished with bad result -> try again
                pev_numberOfCableCheckReq += 1;
                #if 0 /* todo: use config item to decide whether we have inlet voltage measurement or not */
                  publishStatus("CbleChck ongoing", String(hardwareInterface_getInletVoltage()) + "V");
                #else
                  /* no inlet voltage measurement available, just show status */
                  publishStatus("CbleChck ongoing", "");
                #endif
                addToTrace("Will again send CableCheckReq");
                pev_sendCableCheckReq();
                // stay in the same state
                pev_enterState(PEV_STATE_WaitForCableCheckResponse);
            }
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForPreChargeResponse(void) {
  uint16_t u;
  hardwareInterface_simulatePreCharge();
  if (pev_DelayCycles>0) {
    pev_DelayCycles-=1;
    return;
  }
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForPreChargeResponse, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.PreChargeRes_isUsed) {
        addToTrace("PreCharge aknowledge received.");
        EVSEPresentVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Value,
                  dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Multiplier);
        //addToTrace("EVSEPresentVoltage.Value " + String(dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Value));
        //addToTrace("EVSEPresentVoltage.Multiplier " + String(dinDocDec.V2G_Message.Body.PreChargeRes.EVSEPresentVoltage.Multiplier));
        //s = "U_Inlet " + String(hardwareInterface.getInletVoltage()) + "V, "
        //s= s + "U_Accu " + String(hardwareInterface.getAccuVoltage()) + "V"
        #ifdef USE_EVSEPRESENTVOLTAGE_FOR_PRECHARGE_END
          /* use the voltage which is reported by the charger to decide about the end of PreCharge */
          //s = "Using EVSEPresentVoltage=" + String(u);
          //addToTrace(s);
          u = EVSEPresentVoltage;
        #else
          /* use the physically measured inlet voltage to decide about end of PreCharge */
          u = hardwareInterface_getInletVoltage()
        #endif
        if (ABS(u-hardwareInterface_getAccuVoltage()) < PARAM_U_DELTA_MAX_FOR_END_OF_PRECHARGE) {
            addToTrace("Difference between accu voltage and inlet voltage is small. Sending PowerDeliveryReq.");
            publishStatus("PreCharge done", "");
            if (isLightBulbDemo) {
                // For light-bulb-demo, nothing to do here.
                addToTrace("This is a light bulb demo. Do not turn-on the relay at end of precharge.");
            } else {
                // In real-world-case, turn the power relay on.
                hardwareInterface_setPowerRelayOn();
            }
            pev_wasPowerDeliveryRequestedOn=1;
            checkpointNumber = 600;
            pev_sendPowerDeliveryReq(1); /* 1 is ON */
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
        } else {
            publishStatus("PreChrge ongoing", String(u) + "V");
            addToTrace("Difference too big. Continuing PreCharge.");
            pev_sendPreChargeReq();
            pev_DelayCycles=15; // wait with the next evaluation approx half a second
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForContactorsClosed(void) {
  uint8_t readyForNextState=0;
  if (pev_DelayCycles>0) {
        pev_DelayCycles--;
        return;
  }
  if (isLightBulbDemo) {
        readyForNextState=1; /* if it's just a bulb demo, we do not wait for contactor, because it is not switched in this moment. */
  } else {
        readyForNextState = hardwareInterface_getPowerRelayConfirmation(); /* check if the contactor is closed */
        if (readyForNextState) {
            addToTrace("Contactors are confirmed to be closed.");
            publishStatus("Contactors ON", "");
        }
  }
  if (readyForNextState) {
        addToTrace("Sending PowerDeliveryReq.");
        pev_sendPowerDeliveryReq(1); /* 1 is ON */
        pev_wasPowerDeliveryRequestedOn=1;
		pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}


void stateFunctionWaitForPowerDeliveryResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForPowerDeliveryRes, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.PowerDeliveryRes_isUsed) {
        if (pev_wasPowerDeliveryRequestedOn) {
            publishStatus("PwrDelvy ON success", "");
            addToTrace("Checkpoint700: Starting the charging loop with CurrentDemandReq");
            checkpointNumber = 700;
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
        } else {
            /* We requested "OFF". So we turn-off the Relay and continue with the Welding detection. */
            publishStatus("PwrDelvry OFF success", "");
            checkpointNumber = 810;
            /* set the CP line to B */
            hardwareInterface_setStateB();
            addToTrace("Turning off the relay and starting the WeldingDetection");
            hardwareInterface_setPowerRelayOff();
            pev_isBulbOn = 0;
            checkpointNumber = 850;
            pev_sendWeldingDetectionReq();
            pev_enterState(PEV_STATE_WaitForWeldingDetectionResponse);
        }
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForCurrentDemandResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForCurrentDemandRes, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    printf("[%d] step1 %d\r\n", rtc_get_counter_val(), tcp_rxdataLen);
    projectExiConnector_decode_DinExiDocument();

    printf("[%d] step2 %d %d\r\n", rtc_get_counter_val(), g_errn, global_streamDecPos);

    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.CurrentDemandRes_isUsed) {
        /* as long as the accu is not full and no stop-demand from the user, we continue charging */
    	/* If the pushbutton is pressed longer than 0.5s, we interpret this as charge stop request. */
    	pev_isUserStopRequest = pushbutton_tButtonPressTime>(PUSHBUTTON_CYCLES_PER_SECOND/2);
        if (hardwareInterface_getIsAccuFull() || pev_isUserStopRequest) {
            if (hardwareInterface_getIsAccuFull()) {
                publishStatus("Accu full", "");
                addToTrace("Accu is full. Sending PowerDeliveryReq Stop.");
            } else {
                publishStatus("User req stop", "");
                addToTrace("User requested stop. Sending PowerDeliveryReq Stop.");
            }
            pev_wasPowerDeliveryRequestedOn=0;
            checkpointNumber = 800;
            pev_sendPowerDeliveryReq(0);
            pev_enterState(PEV_STATE_WaitForPowerDeliveryResponse);
        } else {
            /* continue charging loop */
            hardwareInterface_simulateCharging();
            #if 0
              u = hardwareInterface_getInletVoltage();
              Param::SetInt(Param::uinlet, hardwareInterface_getInletVoltage());
            #else
              EVSEPresentVoltage = combineValueAndMultiplier(dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentVoltage.Value,
                       dinDocDec.V2G_Message.Body.CurrentDemandRes.EVSEPresentVoltage.Multiplier);
            #endif
            //publishStatus("Charging", String(u) + "V", String(hardwareInterface_getSoc()) + "%");
            Param::SetInt(Param::uevse, EVSEPresentVoltage);
            checkpointNumber = 710;
            pev_sendCurrentDemandReq();
            pev_enterState(PEV_STATE_WaitForCurrentDemandResponse);
        }
    }
  }
  if (isLightBulbDemo) {
      if (pev_cyclesLightBulbDelay<=33*2) {
          pev_cyclesLightBulbDelay+=1;
      } else {
          if (!pev_isBulbOn) {
              addToTrace("This is a light bulb demo. Turning-on the bulb when 2s in the main charging loop.");
              hardwareInterface_setPowerRelayOn();
              pev_isBulbOn = 1;
          }
      }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForWeldingDetectionResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForWeldingDetectionRes, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.WeldingDetectionRes_isUsed) {
        /* todo: add real welding detection here, run in welding detection loop until finished. */
        publishStatus("WldingDet done", "");
        addToTrace("Sending SessionStopReq");
        projectExiConnector_prepare_DinExiDocument();
        dinDocEnc.V2G_Message.Body.SessionStopReq_isUsed = 1u;
        init_dinSessionStopType(&dinDocEnc.V2G_Message.Body.SessionStopReq);
        /* no other fields are manatory */
        checkpointNumber = 900;
        encodeAndTransmit();
        pev_enterState(PEV_STATE_WaitForSessionStopResponse);
    }
  }
  if (pev_isTooLong()) {
      pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionWaitForSessionStopResponse(void) {
  if (tcp_rxdataLen>V2GTP_HEADER_SIZE) {
    addToTrace("In state WaitForSessionStopRes, received:");
    showAsHex(tcp_rxdata, tcp_rxdataLen, "");
    routeDecoderInputData();
    projectExiConnector_decode_DinExiDocument();
    tcp_rxdataLen = 0; /* mark the input data as "consumed" */
    if (dinDocDec.V2G_Message.Body.SessionStopRes_isUsed) {
        // req -508
        // Todo: close the TCP connection here.
        // Todo: Unlock the connector lock.
        publishStatus("Stopped normally", "");
        addToTrace("Charging is finished");
        pev_enterState(PEV_STATE_ChargingFinished);
    }
  }
  if (pev_isTooLong()) {
    pev_enterState(PEV_STATE_SequenceTimeout);
  }
}

void stateFunctionChargingFinished(void) {
  /* charging is finished. */
  /* Finally unlock the connector */
  addToTrace("Charging successfully finished. Unlocking the connector");
  hardwareInterface_triggerConnectorUnlocking();
  pev_enterState(PEV_STATE_End);
}

void stateFunctionSequenceTimeout(void) {
  /* Here we end, if we run into a timeout in the state machine. */
  publishStatus("ERROR Timeout", "");
  /* Initiate the safe-shutdown-sequence. */
  addToTrace("Safe-shutdown-sequence: setting state B");
  checkpointNumber = 1100;
  hardwareInterface_setStateB(); /* setting CP line to B disables in the charger the current flow. */
  pev_DelayCycles = 66; /* 66*30ms=2s for charger shutdown */
  pev_enterState(PEV_STATE_SafeShutDownWaitForChargerShutdown);
}

void stateFunctionUnrecoverableError(void) {
  /* Here we end, if the EVSE reported an error code, which terminates the charging session. */
  publishStatus("ERROR reported", "");
  /* Initiate the safe-shutdown-sequence. */
  addToTrace("Safe-shutdown-sequence: setting state B");
  checkpointNumber = 1200;
  hardwareInterface_setStateB(); /* setting CP line to B disables in the charger the current flow. */
  pev_DelayCycles = 66; /* 66*30ms=2s for charger shutdown */
  pev_enterState(PEV_STATE_SafeShutDownWaitForChargerShutdown);
}

void stateFunctionSafeShutDownWaitForChargerShutdown(void) {
  /* wait state, to give the charger the time to stop the current. */
  if (pev_DelayCycles>0) {
      pev_DelayCycles--;
      return;
  }
  /* Now the current flow is stopped by the charger. We can safely open the contactors: */
  addToTrace("Safe-shutdown-sequence: opening contactors");
  checkpointNumber = 1300;
  hardwareInterface_setPowerRelayOff();
  pev_DelayCycles = 33; /* 33*30ms=1s for opening the contactors */
  pev_enterState(PEV_STATE_SafeShutDownWaitForContactorsOpen);
}

void stateFunctionSafeShutDownWaitForContactorsOpen(void) {
  /* wait state, to give the contactors the time to open. */
  if (pev_DelayCycles>0) {
      pev_DelayCycles--;
      return;
  }
  /* Finally, when we have no current and no voltage, unlock the connector */
  checkpointNumber = 1400;
  addToTrace("Safe-shutdown-sequence: unlocking the connector");
  hardwareInterface_triggerConnectorUnlocking();
  /* This is the end of the safe-shutdown-sequence. */
  pev_enterState(PEV_STATE_End);
}

void stateFunctionEnd(void) {
  /* Just stay here, until we get re-initialized after a new SLAC/SDP. */
}

void pev_enterState(uint16_t n) {
  printf("[%d] [PEV] from %d entering %d\r\n", rtc_get_counter_val(), pev_state, n);
  pev_state = n;
  pev_cyclesInState = 0;
}

uint8_t pev_isTooLong(void) {
  uint16_t limit;
  /* The timeout handling function. */
  limit = 66; /* number of call cycles until timeout. Default 66 cycles with 30ms, means approx. 2 seconds.
        This 2s is the specified timeout time for many messages, fitting to the
        performance time of 1.5s. Exceptions see below. */
  if (pev_state==PEV_STATE_WaitForChargeParameterDiscoveryResponse) {
    limit = 5*33; /* On some charger models, the chargeParameterDiscovery needs more than a second. Wait at least 5s. */
  }
  if (pev_state==PEV_STATE_WaitForCableCheckResponse) {
    limit = 30*33; // CableCheck may need some time. Wait at least 30s.
  }
  if (pev_state==PEV_STATE_WaitForPreChargeResponse) {
    limit = 30*33; // PreCharge may need some time. Wait at least 30s.
  }
  if (pev_state==PEV_STATE_WaitForPowerDeliveryResponse) {
    limit = 6*33; /* PowerDelivery may need some time. Wait at least 6s. On Compleo charger, observed more than 1s until response.
                     specified performance time is 4.5s (ISO) */
  }
  if (pev_state==PEV_STATE_WaitForCurrentDemandResponse) {
    limit = 5*33;  /* Test with 5s timeout. Just experimental.
                      The specified performance time is 25ms (ISO), the specified timeout 250ms. */
  }
  return (pev_cyclesInState > limit);
}

/******* The statemachine dispatcher *******************/
void pev_runFsm(void) {
 if (connMgr_getConnectionLevel()<CONNLEVEL_80_TCP_RUNNING) {
        /* If we have no TCP to the charger, nothing to do here. Just wait for the link. */
        if (pev_state!=PEV_STATE_NotYetInitialized) {
            pev_enterState(PEV_STATE_NotYetInitialized);
            hardwareInterface_setStateB();
            hardwareInterface_setPowerRelayOff();
            pev_isBulbOn = 0;
            pev_cyclesLightBulbDelay = 0;
        }
        return;
 }
 if (connMgr_getConnectionLevel()==CONNLEVEL_80_TCP_RUNNING) {
        /* We have a TCP connection. This is the trigger for us. */
        if (pev_state==PEV_STATE_NotYetInitialized) pev_enterState(PEV_STATE_Connected);
 }
 switch (pev_state) {
    case PEV_STATE_Connected: stateFunctionConnected();  break;
    case PEV_STATE_WaitForSupportedApplicationProtocolResponse: stateFunctionWaitForSupportedApplicationProtocolResponse(); break;
    case PEV_STATE_WaitForSessionSetupResponse: stateFunctionWaitForSessionSetupResponse(); break;
    case PEV_STATE_WaitForServiceDiscoveryResponse: stateFunctionWaitForServiceDiscoveryResponse(); break;
    case PEV_STATE_WaitForServicePaymentSelectionResponse: stateFunctionWaitForServicePaymentSelectionResponse(); break;
    case PEV_STATE_WaitForContractAuthenticationResponse: stateFunctionWaitForContractAuthenticationResponse(); break;
    case PEV_STATE_WaitForChargeParameterDiscoveryResponse: stateFunctionWaitForChargeParameterDiscoveryResponse(); break;
    case PEV_STATE_WaitForConnectorLock: stateFunctionWaitForConnectorLock(); break;
    case PEV_STATE_WaitForCableCheckResponse: stateFunctionWaitForCableCheckResponse(); break;
    case PEV_STATE_WaitForPreChargeResponse: stateFunctionWaitForPreChargeResponse(); break;
    case PEV_STATE_WaitForContactorsClosed: stateFunctionWaitForContactorsClosed(); break;
    case PEV_STATE_WaitForPowerDeliveryResponse: stateFunctionWaitForPowerDeliveryResponse(); break;
    case PEV_STATE_WaitForCurrentDemandResponse: stateFunctionWaitForCurrentDemandResponse(); break;
    case PEV_STATE_WaitForWeldingDetectionResponse: stateFunctionWaitForWeldingDetectionResponse(); break;
    case PEV_STATE_WaitForSessionStopResponse: stateFunctionWaitForSessionStopResponse(); break;
    case PEV_STATE_ChargingFinished: stateFunctionChargingFinished(); break;
    case PEV_STATE_UnrecoverableError: stateFunctionUnrecoverableError(); break;
    case PEV_STATE_SequenceTimeout: stateFunctionSequenceTimeout(); break;
    case PEV_STATE_SafeShutDownWaitForChargerShutdown: stateFunctionSafeShutDownWaitForChargerShutdown(); break;
    case PEV_STATE_SafeShutDownWaitForContactorsOpen: stateFunctionSafeShutDownWaitForContactorsOpen(); break;
    case PEV_STATE_End: stateFunctionEnd(); break;
 }
}

/************ public interfaces *****************************************/
/* The init function for the PEV charging state machine. */
void pevStateMachine_Init(void) {
  pev_state=PEV_STATE_NotYetInitialized;
}

void pevStateMachine_ReInit(void) {
  addToTrace("re-initializing fsmPev");
  tcp_Disconnect();
  hardwareInterface_setStateB();
  hardwareInterface_setPowerRelayOff();
  pev_isBulbOn = 0;
  pev_cyclesLightBulbDelay = 0;
  pev_state = PEV_STATE_Connecting;
  pev_cyclesInState = 0;
}

/* The cyclic main function of the PEV charging state machine.
   Called each 30ms. */
void pevStateMachine_Mainfunction(void) {
    // run the state machine:
    pev_cyclesInState += 1; // for timeout handling, count how long we are in a state
    pev_runFsm();
}
