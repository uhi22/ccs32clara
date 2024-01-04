# Bug analysis

## 2024-01-02 LEDs do not work

### Symptoms
- Precondition: mass-erasing and flashing the bootloader and the application via STLink.
- Observed: the RGB LEDs are off.
- Expected: Green LED is on, and when searching the modem (each 5s), the blue and red additionally turn shortly on.

### History check
- https://github.com/uhi22/ccs32clara/commit/8c0c7e3c2d687d9151cb513a45e73fd74fc2b0b5 still fine.
- https://github.com/uhi22/ccs32clara/commit/403cd776b2dfda78039a6912241a0408873599e0 (The eth buffer optimization): different blink pattern. Long white.
- https://github.com/uhi22/ccs32clara/commit/039646fcb0a11f875899947696ca313a075fe670 Fine again.
- https://github.com/uhi22/ccs32clara/commit/09aeefe6f217de3be0640e730f265c143438f92b () LEDs completely off.

### Root cause
handleApplicationRGBLeds() is only called if actuator test is active.

### Solution
call handleApplicationRGBLeds() if actuator test is NOT running. Fixed in https://github.com/uhi22/ccs32clara/commit/a5ccab85edd307058041ce9bf600308470f350f7


## 2023-12-10 Ionity (Tritium) chargers do not work
- Preconditions: charging session is started
- Observed: specified current is delivered for a few seconds, then session stops and restarts: https://openinverter.org/forum/viewtopic.php?p=64655#p64655
- Expected: charging should just continue

### Root cause
- Charger overvoltage limit was set to a too low constant value (398V at 393V batt voltage)

### Solution
- Made limits configurable and default to 410V d77fd165d365c3edae2228c7bc3687a6d682bdf1

## 2023-12-10 Contactors open too quickly

### Symptoms
- Precondition: running charge session
- Oberserved: when stopping session on chargers it seems contactors open with no or very little delay, possibly still conducting current https://openinverter.org/forum/viewtopic.php?p=64655#p64655
- Expected: Current should ramp down to 0A then after some delay contactors should open

### Root cause
- PEV state machine went straight to stop state without checking current

### Solution
- Add states to gracefully stop charging then open contactors 9b50656761a4a39cd6d187e6cfb489f9c9560f96

## 2023-12-08 ABB chargers do not work 2

### Symptoms
- Observed: the charger would go straight into fault state https://openinverter.org/forum/viewtopic.php?p=64541#p64541
- Expected: the charger should go into charging state

### Root cause
- stateC was always asserted https://openinverter.org/forum/viewtopic.php?p=64557#p64557

### Solution
- Make sure stateC is reset at startup and in boot loader phase a9e65c1363d0014860f1685a21baf9aa02a02189

## 2023-12-08 When Tesla SC stops session Clara stays in current delivery

### Symptoms
- Precondition: A charging session runs fine at Tesla SC
- Oberserved: when stopping the session in the Tesla app Clara stays in CurrentDelivery and consequently doesn't open the contactors: https://openinverter.org/forum/viewtopic.php?p=64563#p64563
- Expected: when stopping on charger Clara should gracefully stop the charging session and open contactors

### Root cause
- EVSE shutdown was not evaluated

### Solution
- Evaluate EVSE shutdown and stop session when indicated f5e66dfc7c9d7a0768ba0da688e2d46a5787ccc4

## 2023-11-29 ABB chargers do not work 1

### Symptoms
- Observed: Charger charges briefly then goes into fault state https://openinverter.org/forum/viewtopic.php?p=64234#p64234
- Expected: the charger should keep charging

### Root cause
- Most likely because TCP retries were not implemented

### Solution
- Implement retries ed142b298b09cabddf2f12804eab443c913896b0


