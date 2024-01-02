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

