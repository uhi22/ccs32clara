/* Interface header for pevStateMachine.c */

/* Global Defines */

#define V2GTP_HEADER_SIZE 8 /* The V2GTP header has 8 bytes */
#define MAX_LABEL_LEN     25

/* Global Variables */
extern const char pevSttLabelsIso1[][MAX_LABEL_LEN];

/* Global Functions */
#ifdef __cplusplus
extern "C" {
#endif
/* pev state machine for ISO */
extern void pevStateMachineISO1_Mainfunction(void);
extern void pevStateMachineISO1_Start(void);

#ifdef __cplusplus
}
#endif
