/* ISO 15118-2:2013 messages for a DC session with external payment (EIM). The state machine in pevStateMachine.cpp
   decides what to send and when; this only builds the requests and reads the responses (see pevMessages.h).
   The values sent are the same as for DIN 70121 (pevStateMachine.cpp). */

#include "ccs32_globals.h"
#include "projectExiConnector.h"
#include "pevMessages.h"

#ifdef USE_ISO1

#define req (ExiDocEnc.iso1D.V2G_Message.Body)
#define res (ExiDocDec.iso1D.V2G_Message.Body)

static float scaled(const struct iso1PhysicalValueType& v)
{
   return pev_scaledValue(v.Value, v.Multiplier);
}

static void setValue(struct iso1PhysicalValueType* v, int16_t value, int8_t multiplier, iso1unitSymbolType unit)
{
   v->Value = value;
   v->Multiplier = multiplier;
   v->Unit = unit;
}

/* DC_EVStatus as DIN sends it: no error, the state of charge, and EVReady */
static void setEvStatus(struct iso1DC_EVStatusType* st, int ready)
{
   st->EVReady = ready;
   st->EVErrorCode = iso1DC_EVErrorCodeType_NO_ERROR;
   st->EVRESSSOC = hardwareInterface_getSoc();
}

/* OK, OK_NewSessionEstablished, OK_OldSessionJoined and OK_CertificateExpiresSoon go on; FAILED... stop */
static void readResponseCode(struct pevResponse* r, iso1responseCodeType code)
{
   r->responseCode = (uint8_t)code;
   r->isResponseCodeOk = code < iso1responseCodeType_FAILED;
   r->isResponseFailed = !r->isResponseCodeOk;
}

static void readEvseStatus(struct pevResponse* r, const struct iso1DC_EVSEStatusType* st)
{
   r->evseStatusCode = (uint8_t)st->EVSEStatusCode;
   r->isChargerStopRequested = st->EVSEStatusCode == iso1DC_EVSEStatusCodeType_EVSE_Shutdown
                               || st->EVSENotification == iso1EVSENotificationType_StopCharging;
}

void pevIso1_readResponse(struct pevResponse* r)
{
   memset(r, 0, sizeof(*r));
   if (res.SessionSetupRes_isUsed)
   {
      r->type = PEV_RES_SESSIONSETUP;
      readResponseCode(r, res.SessionSetupRes.ResponseCode);
      r->sessionIdLen = (uint8_t)ExiDocDec.iso1D.V2G_Message.Header.SessionID.bytesLen;
      if (r->sessionIdLen > sizeof(r->sessionId))
         r->sessionIdLen = sizeof(r->sessionId);
      memcpy(r->sessionId, ExiDocDec.iso1D.V2G_Message.Header.SessionID.bytes, r->sessionIdLen);
   }
   else if (res.ServiceDiscoveryRes_isUsed)
   {
      const struct iso1ServiceDiscoveryResType* sd = &res.ServiceDiscoveryRes;
      r->type = PEV_RES_SERVICEDISCOVERY;
      readResponseCode(r, sd->ResponseCode);
      for (uint16_t i = 0; i < sd->PaymentOptionList.PaymentOption.arrayLen; i++)
      {
         if (sd->PaymentOptionList.PaymentOption.array[i] == iso1paymentOptionType_ExternalPayment)
            r->isExternalPaymentOffered = 1;
      }
      r->chargeServiceId = sd->ChargeService.ServiceID;
      for (uint16_t i = 0; i < sd->ChargeService.SupportedEnergyTransferMode.EnergyTransferMode.arrayLen; i++)
      {
         if (sd->ChargeService.SupportedEnergyTransferMode.EnergyTransferMode.array[i] == iso1EnergyTransferModeType_DC_extended)
            r->isDcExtendedOffered = 1;
      }
   }
   else if (res.PaymentServiceSelectionRes_isUsed)
   {
      r->type = PEV_RES_PAYMENTSELECTION;
      readResponseCode(r, res.PaymentServiceSelectionRes.ResponseCode);
   }
   else if (res.AuthorizationRes_isUsed)
   {
      r->type = PEV_RES_AUTHORIZATION;
      readResponseCode(r, res.AuthorizationRes.ResponseCode);
      r->isProcessingFinished = res.AuthorizationRes.EVSEProcessing == iso1EVSEProcessingType_Finished;
   }
   else if (res.ChargeParameterDiscoveryRes_isUsed)
   {
      const struct iso1ChargeParameterDiscoveryResType* cpd = &res.ChargeParameterDiscoveryRes;
      r->type = PEV_RES_CHARGEPARAMETERDISCOVERY;
      readResponseCode(r, cpd->ResponseCode);
      r->isProcessingFinished = cpd->EVSEProcessing == iso1EVSEProcessingType_Finished;
      r->maxVoltage = scaled(cpd->DC_EVSEChargeParameter.EVSEMaximumVoltageLimit);
      r->maxCurrent = scaled(cpd->DC_EVSEChargeParameter.EVSEMaximumCurrentLimit);
      r->minVoltage = scaled(cpd->DC_EVSEChargeParameter.EVSEMinimumVoltageLimit);
      if (cpd->DC_EVSEChargeParameter_isUsed)
         readEvseStatus(r, &cpd->DC_EVSEChargeParameter.DC_EVSEStatus);
      if (cpd->SAScheduleList_isUsed && cpd->SAScheduleList.SAScheduleTuple.arrayLen > 0)
      {
         r->hasSaSchedule = 1;
         r->saScheduleTupleId = cpd->SAScheduleList.SAScheduleTuple.array[0].SAScheduleTupleID;
      }
   }
   else if (res.CableCheckRes_isUsed)
   {
      r->type = PEV_RES_CABLECHECK;
      readResponseCode(r, res.CableCheckRes.ResponseCode);
      r->isProcessingFinished = res.CableCheckRes.EVSEProcessing == iso1EVSEProcessingType_Finished;
      readEvseStatus(r, &res.CableCheckRes.DC_EVSEStatus);
   }
   else if (res.PreChargeRes_isUsed)
   {
      r->type = PEV_RES_PRECHARGE;
      readResponseCode(r, res.PreChargeRes.ResponseCode);
      r->presentVoltage = scaled(res.PreChargeRes.EVSEPresentVoltage);
      readEvseStatus(r, &res.PreChargeRes.DC_EVSEStatus);
   }
   else if (res.PowerDeliveryRes_isUsed)
   {
      r->type = PEV_RES_POWERDELIVERY;
      readResponseCode(r, res.PowerDeliveryRes.ResponseCode);
      if (res.PowerDeliveryRes.DC_EVSEStatus_isUsed)
         readEvseStatus(r, &res.PowerDeliveryRes.DC_EVSEStatus);
   }
   else if (res.CurrentDemandRes_isUsed)
   {
      r->type = PEV_RES_CURRENTDEMAND;
      readResponseCode(r, res.CurrentDemandRes.ResponseCode);
      r->presentVoltage = scaled(res.CurrentDemandRes.EVSEPresentVoltage);
      r->presentCurrent = scaled(res.CurrentDemandRes.EVSEPresentCurrent);
      readEvseStatus(r, &res.CurrentDemandRes.DC_EVSEStatus);
      /* ReceiptRequired is ignored: with external payment there is no MeteringReceipt */
   }
   else if (res.WeldingDetectionRes_isUsed)
   {
      r->type = PEV_RES_WELDINGDETECTION;
      readResponseCode(r, res.WeldingDetectionRes.ResponseCode);
      r->presentVoltage = scaled(res.WeldingDetectionRes.EVSEPresentVoltage);
      readEvseStatus(r, &res.WeldingDetectionRes.DC_EVSEStatus);
   }
   else if (res.SessionStopRes_isUsed)
   {
      r->type = PEV_RES_SESSIONSTOP;
      readResponseCode(r, res.SessionStopRes.ResponseCode);
   }
}

void pevIso1_prepareSessionSetupReq(const uint8_t* evccId, uint8_t evccIdLen)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   /* SessionID zero: a new session (8 bytes, as for DIN) */
   memset(ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytes, 0, 8);
   ExiDocEnc.iso1D.V2G_Message.Header.SessionID.bytesLen = 8;
   req.SessionSetupReq_isUsed = 1u;
   init_iso1SessionSetupReqType(&req.SessionSetupReq);
   if (evccIdLen > iso1SessionSetupReqType_EVCCID_BYTES_SIZE)
      evccIdLen = iso1SessionSetupReqType_EVCCID_BYTES_SIZE;
   memcpy(req.SessionSetupReq.EVCCID.bytes, evccId, evccIdLen); /* the EVCC's MAC address */
   req.SessionSetupReq.EVCCID.bytesLen = evccIdLen;
}

void pevIso1_prepareServiceDiscoveryReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.ServiceDiscoveryReq_isUsed = 1u;
   init_iso1ServiceDiscoveryReqType(&req.ServiceDiscoveryReq);
   req.ServiceDiscoveryReq.ServiceCategory = iso1serviceCategoryType_EVCharging; /* charging only, no other services */
   req.ServiceDiscoveryReq.ServiceCategory_isUsed = 1u;
}

void pevIso1_preparePaymentServiceSelectionReq(uint16_t chargeServiceId)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.PaymentServiceSelectionReq_isUsed = 1u;
   init_iso1PaymentServiceSelectionReqType(&req.PaymentServiceSelectionReq);
   req.PaymentServiceSelectionReq.SelectedPaymentOption = iso1paymentOptionType_ExternalPayment;
   /* only the charge service */
   req.PaymentServiceSelectionReq.SelectedServiceList.SelectedService.array[0].ServiceID = chargeServiceId;
   req.PaymentServiceSelectionReq.SelectedServiceList.SelectedService.array[0].ParameterSetID_isUsed = 0u;
   req.PaymentServiceSelectionReq.SelectedServiceList.SelectedService.arrayLen = 1;
}

void pevIso1_prepareAuthorizationReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.AuthorizationReq_isUsed = 1u;
   init_iso1AuthorizationReqType(&req.AuthorizationReq); /* no Id and no GenChallenge: external payment */
}

void pevIso1_prepareChargeParameterDiscoveryReq(void)
{
   struct iso1DC_EVChargeParameterType* cp;

   projectExiConnector_prepare_Iso1ExiDocument();
   req.ChargeParameterDiscoveryReq_isUsed = 1u;
   init_iso1ChargeParameterDiscoveryReqType(&req.ChargeParameterDiscoveryReq);
   req.ChargeParameterDiscoveryReq.RequestedEnergyTransferMode = iso1EnergyTransferModeType_DC_extended;
   /* no more PMaxSchedule entries than the decoder can hold */
   req.ChargeParameterDiscoveryReq.MaxEntriesSAScheduleTuple = iso1PMaxScheduleType_PMaxScheduleEntry_ARRAY_SIZE;
   req.ChargeParameterDiscoveryReq.MaxEntriesSAScheduleTuple_isUsed = 1u;
   req.ChargeParameterDiscoveryReq.DC_EVChargeParameter_isUsed = 1u;
   cp = &req.ChargeParameterDiscoveryReq.DC_EVChargeParameter;
   init_iso1DC_EVChargeParameterType(cp);
   setEvStatus(&cp->DC_EVStatus, 0); /* EVReady 0 here, as for DIN */
   setValue(&cp->EVMaximumCurrentLimit, Param::GetInt(Param::MaxCurrent), 0, iso1unitSymbolType_A);
   setValue(&cp->EVMaximumPowerLimit, Param::GetInt(Param::MaxPower) * 10, 2, iso1unitSymbolType_W); /* kW: x10 x 10^2 */
   cp->EVMaximumPowerLimit_isUsed = 1u;
   setValue(&cp->EVMaximumVoltageLimit, Param::GetInt(Param::MaxVoltage), 0, iso1unitSymbolType_V);
   setValue(&cp->EVEnergyCapacity, 10000, 1, iso1unitSymbolType_Wh); /* 100 kWh, so that it doesn't get in the way */
   cp->EVEnergyCapacity_isUsed = 1u;
   setValue(&cp->EVEnergyRequest, 10000, 1, iso1unitSymbolType_Wh);
   cp->EVEnergyRequest_isUsed = 1u;
   cp->FullSOC = 100;
   cp->FullSOC_isUsed = 1u;
   cp->BulkSOC = 80;
   cp->BulkSOC_isUsed = 1u;
}

void pevIso1_prepareCableCheckReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.CableCheckReq_isUsed = 1u;
   init_iso1CableCheckReqType(&req.CableCheckReq);
   setEvStatus(&req.CableCheckReq.DC_EVStatus, 1);
}

void pevIso1_preparePreChargeReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.PreChargeReq_isUsed = 1u;
   init_iso1PreChargeReqType(&req.PreChargeReq);
   setEvStatus(&req.PreChargeReq.DC_EVStatus, 1);
   setValue(&req.PreChargeReq.EVTargetVoltage, hardwareInterface_getAccuVoltage(), 0, iso1unitSymbolType_V);
   setValue(&req.PreChargeReq.EVTargetCurrent, 1, 0, iso1unitSymbolType_A); /* 1A for precharging */
}

void pevIso1_preparePowerDeliveryReq(uint8_t isOn, uint8_t saScheduleTupleId)
{
   struct iso1DC_EVPowerDeliveryParameterType* pd;

   projectExiConnector_prepare_Iso1ExiDocument();
   req.PowerDeliveryReq_isUsed = 1u;
   init_iso1PowerDeliveryReqType(&req.PowerDeliveryReq);
   req.PowerDeliveryReq.ChargeProgress = isOn ? iso1chargeProgressType_Start : iso1chargeProgressType_Stop;
   req.PowerDeliveryReq.SAScheduleTupleID = saScheduleTupleId;
   req.PowerDeliveryReq.DC_EVPowerDeliveryParameter_isUsed = 1u;
   pd = &req.PowerDeliveryReq.DC_EVPowerDeliveryParameter;
   init_iso1DC_EVPowerDeliveryParameterType(pd);
   setEvStatus(&pd->DC_EVStatus, 1);
   pd->ChargingComplete = 0;
   pd->BulkChargingComplete = 0;
   pd->BulkChargingComplete_isUsed = 1u;
}

void pevIso1_prepareCurrentDemandReq(int16_t targetVoltage)
{
   struct iso1CurrentDemandReqType* cd = &req.CurrentDemandReq;

   projectExiConnector_prepare_Iso1ExiDocument();
   req.CurrentDemandReq_isUsed = 1u;
   init_iso1CurrentDemandReqType(cd);
   setEvStatus(&cd->DC_EVStatus, 1);
   setValue(&cd->EVTargetVoltage, targetVoltage, 0, iso1unitSymbolType_V);
   setValue(&cd->EVTargetCurrent, hardwareInterface_getChargingTargetCurrent(), 0, iso1unitSymbolType_A);
   cd->ChargingComplete = 0;
   cd->BulkChargingComplete = 0;
   cd->BulkChargingComplete_isUsed = 1u;
   setValue(&cd->RemainingTimeToFullSoC, 1200, 0, iso1unitSymbolType_s);
   cd->RemainingTimeToFullSoC_isUsed = 1u;
   setValue(&cd->RemainingTimeToBulkSoC, 600, 0, iso1unitSymbolType_s);
   cd->RemainingTimeToBulkSoC_isUsed = 1u;
   setValue(&cd->EVMaximumVoltageLimit, Param::GetInt(Param::MaxVoltage), 0, iso1unitSymbolType_V);
   cd->EVMaximumVoltageLimit_isUsed = 1u;
   setValue(&cd->EVMaximumCurrentLimit, Param::GetInt(Param::MaxCurrent), 0, iso1unitSymbolType_A);
   cd->EVMaximumCurrentLimit_isUsed = 1u;
}

void pevIso1_prepareWeldingDetectionReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.WeldingDetectionReq_isUsed = 1u;
   init_iso1WeldingDetectionReqType(&req.WeldingDetectionReq);
   setEvStatus(&req.WeldingDetectionReq.DC_EVStatus, 1);
}

void pevIso1_prepareSessionStopReq(void)
{
   projectExiConnector_prepare_Iso1ExiDocument();
   req.SessionStopReq_isUsed = 1u;
   init_iso1SessionStopReqType(&req.SessionStopReq);
   req.SessionStopReq.ChargingSession = iso1chargingSessionType_Terminate;
}

#endif
