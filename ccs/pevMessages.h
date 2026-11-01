/* The charging state machine (pevStateMachine.cpp) runs the same sequence for DIN 70121 and ISO 15118-2. What
   differs is only how the messages look. A decoded response is read into this protocol-neutral form, right after
   decoding, and the state machine decides on it; the requests are built by protocol-specific functions. */

#ifndef PEVMESSAGES_H
#define PEVMESSAGES_H

#include <stdint.h>

/* Which response was received */
enum pevResponseType
{
   PEV_RES_NONE,
   PEV_RES_SESSIONSETUP,
   PEV_RES_SERVICEDISCOVERY,
   PEV_RES_PAYMENTSELECTION,   /* DIN ServicePaymentSelectionRes, ISO PaymentServiceSelectionRes */
   PEV_RES_AUTHORIZATION,      /* DIN ContractAuthenticationRes, ISO AuthorizationRes */
   PEV_RES_CHARGEPARAMETERDISCOVERY,
   PEV_RES_CABLECHECK,
   PEV_RES_PRECHARGE,
   PEV_RES_POWERDELIVERY,
   PEV_RES_CURRENTDEMAND,
   PEV_RES_WELDINGDETECTION,
   PEV_RES_SESSIONSTOP
};

/* DC_EVSEStatusCode: DIN 70121 and ISO 15118-2 number it the same way */
#define PEV_EVSESTATUS_NOTREADY             0
#define PEV_EVSESTATUS_READY                1
#define PEV_EVSESTATUS_SHUTDOWN             2
#define PEV_EVSESTATUS_UTILITYINTERRUPT     3
#define PEV_EVSESTATUS_ISOLATIONMONITORING  4
#define PEV_EVSESTATUS_EMERGENCYSHUTDOWN    5
#define PEV_EVSESTATUS_MALFUNCTION          6

struct pevResponse
{
   uint8_t type;                 /* enum pevResponseType */
   uint8_t isResponseCodeOk;     /* ResponseCode is OK (DIN: CableCheckRes only; ISO: OK or OK_...) */
   uint8_t isResponseFailed;     /* ISO only: ResponseCode is FAILED or FAILED_...; DIN response codes aren't read */
   uint8_t responseCode;         /* ISO only: the ResponseCode */
   uint8_t isProcessingFinished; /* EVSEProcessing is Finished */
   uint8_t evseStatusCode;       /* PEV_EVSESTATUS_... */
   uint8_t isChargerStopRequested; /* the charger asks to stop (EVSE_Shutdown; with ISO also EVSENotification StopCharging) */
   float presentVoltage;         /* EVSEPresentVoltage, V */
   float presentCurrent;         /* EVSEPresentCurrent, A */
   float maxVoltage;             /* EVSEMaximumVoltageLimit, V */
   float maxCurrent;             /* EVSEMaximumCurrentLimit, A */
   float minVoltage;             /* EVSEMinimumVoltageLimit, V */
   uint8_t sessionId[8];         /* SessionSetupRes: the session ID from the header */
   uint8_t sessionIdLen;
   /* ISO only */
   uint8_t isExternalPaymentOffered; /* ServiceDiscoveryRes */
   uint8_t isDcExtendedOffered;      /* ServiceDiscoveryRes: the charge service offers DC_extended */
   uint16_t chargeServiceId;         /* ServiceDiscoveryRes */
   uint8_t hasSaSchedule;            /* ChargeParameterDiscoveryRes has a SAScheduleList */
   uint8_t saScheduleTupleId;        /* its first SAScheduleTuple */
};

/* Value with a power-of-ten multiplier, as V2G messages carry physical values */
float pev_scaledValue(int32_t value, int8_t multiplier);

#ifdef USE_ISO1
/* ISO 15118-2:2013 messages (pevIso1.cpp). Each prepare function fills the encoder document; the state machine then
   encodes and sends it. */
void pevIso1_readResponse(struct pevResponse* r);
void pevIso1_prepareSessionSetupReq(const uint8_t* evccId, uint8_t evccIdLen);
void pevIso1_prepareServiceDiscoveryReq(void);
void pevIso1_preparePaymentServiceSelectionReq(uint16_t chargeServiceId);
void pevIso1_prepareAuthorizationReq(void);
void pevIso1_prepareChargeParameterDiscoveryReq(void);
void pevIso1_prepareCableCheckReq(void);
void pevIso1_preparePreChargeReq(void);
void pevIso1_preparePowerDeliveryReq(uint8_t isOn, uint8_t saScheduleTupleId);
void pevIso1_prepareCurrentDemandReq(int16_t targetVoltage);
void pevIso1_prepareWeldingDetectionReq(void);
void pevIso1_prepareSessionStopReq(void);
#endif

#endif
