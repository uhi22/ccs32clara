/* Interface header for hardwareInterface.c */

/* Global Defines */

/* Global Variables */
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim2;

extern volatile uint32_t cpDuty_Percent;
extern volatile uint32_t cpFrequency_Hz;

/* Global Functions */

extern void hardwareInterface_setStateB(void);
extern void hardwareInterface_setStateC(void);
extern void hardwareInterface_setRelay2Off(void);
extern void hardwareInterface_setRelay2On(void);
extern void hardwareInterface_setPowerRelayOff(void);
extern void hardwareInterface_setPowerRelayOn(void);
extern void hardwareInterface_setRGB(uint8_t rgb);
extern void hardwareInterface_triggerConnectorLocking(void);
extern void hardwareInterface_triggerConnectorUnlocking(void);
extern uint8_t hardwareInterface_isConnectorLocked(void);
extern uint8_t hardwareInterface_getPowerRelayConfirmation(void);

extern uint8_t hardwareInterface_getIsAccuFull(void);
extern uint8_t hardwareInterface_getSoc(void);
extern void hardwareInterface_simulateCharging(void);
extern void hardwareInterface_resetSimulation(void);
extern void hardwareInterface_simulatePreCharge(void);
extern int16_t hardwareInterface_getAccuVoltage(void);
extern int16_t hardwareInterface_getChargingTargetVoltage(void);
extern int16_t hardwareInterface_getChargingTargetCurrent(void);

extern void hardwareInterface_cyclic(void);
extern void hardwareInterface_init(void);


