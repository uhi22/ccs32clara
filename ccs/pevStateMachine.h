/* Interface header for pevStateMachine.c */

/* Global Defines */

#define V2GTP_HEADER_SIZE 8 /* The V2GTP header has 8 bytes */
#define MAX_LABEL_LEN     25

/* Global Variables */
extern const char pevSttLabels[][MAX_LABEL_LEN];

/* Global Functions */
#ifdef __cplusplus
extern "C" {
#endif
/* pev state machine */
extern void pevStateMachine_ReInit(void);
extern void pevStateMachine_Mainfunction(void);
/* 1 if the car offers DIN 70121 (V2gProtocols 0, 1 or 2, or a build without ISO 15118-2) */
extern uint8_t pevStateMachine_isDinOffered(void);

#ifdef __cplusplus
}
#endif
