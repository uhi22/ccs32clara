/* Interface header for AC Onboard Charger handling */

/* Global Defines */

#define RGB_OFF 0
#define RGB_RED 1
#define RGB_GREEN 2
#define RGB_BLUE 4
#define RGB_CYAN 6
#define RGB_WHITE 7

/* Global Variables */


/* Global Functions */

extern void acOBC_mainfunction(void);
extern uint8_t acOBC_getRGB(void);
extern uint8_t acOBC_isBasicAcCharging(void);




