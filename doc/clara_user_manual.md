# ccs32clara User Manual

This manual is about the CCS charge controller, based on hardware "foccci" and software "ccs32clara".

https://github.com/uhi22/foccci

https://github.com/uhi22/ccs32clara

## RGB Blink Patterns

- White (R+G+B) solid: modem sleep or modem search. If there is no homeplug coordinator (charger or other homeplug network) detected, the homeplug modem enters a power-saving standby quite fast, approx 30 seconds after power-on or 30 seconds after the last seen homeplug traffic. As soon as a homeplug coordinator is detected, the modem wakes up automatically.
- Green solid: modem awake, waiting for plug
- Green slow blink: SLAC ongoing
- Green fast blinking: high-level protocol handshake
- Blue fast blinking: charge preparation
- Blue solid: charging
- Green/Blue fast blinking: charge finished
- Red: Error
