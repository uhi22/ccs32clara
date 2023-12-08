/* Interface header for pevStateMachine.c */

/* Global Defines */

#define V2GTP_HEADER_SIZE 8 /* The V2GTP header has 8 bytes */

/* Global Variables */
extern uint16_t EVSEPresentVoltage;

/* Global Functions */
#ifdef __cplusplus
extern "C" {
#endif
/* pev state machine */
extern void pevStateMachine_ReInit(void);
extern void pevStateMachine_Mainfunction(void);

#ifdef __cplusplus
}
#endif
