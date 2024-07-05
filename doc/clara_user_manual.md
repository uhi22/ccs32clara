# ccs32clara User Manual

This manual is about the CCS charge controller, based on hardware "foccci" and software "ccs32clara".

https://github.com/uhi22/foccci

https://github.com/uhi22/ccs32clara

https://github.com/jsphuebner/ccs32clara

## RGB Blink Patterns

- White (R+G+B) solid: modem sleep or modem search. If there is no homeplug coordinator (charger or other homeplug network) detected, the homeplug modem enters a power-saving standby quite fast, approx 30 seconds after power-on or 30 seconds after the last seen homeplug traffic. As soon as a homeplug coordinator is detected, the modem wakes up automatically.
- Green solid: modem awake, waiting for plug
- Green slow blink: SLAC ongoing
- Green fast blinking: high-level protocol handshake
- Blue fast blinking: cable check
- Blue very fast flashing: pre charge
- Blue solid: charging
- Green/Blue fast blinking: charge finished
- Red: Error. Clara will leave the error state after some seconds and re-try a connection. For finding the cause of the error, observe the spot value lasterr and/or check the console log.
- Red fast blinking: Clara detected a CP PWM which indicates AC charging, but does not see CAN communication from the AC OBC. Make sure that you have the correct CAN mapping and correct value for AcObcState.

## Serial logging

Clara is talking a lot. For trouble-shooting it helps to listen to hers serial port. Connect an Serial-to-USB (e.g. FT232-like) to the TX pin,
and start a serial terminal (e.g. Putty or the Arduino serial console) with 921600 Baud. While Clara is alone, we should see something like this (during the first 30 seconds after startup). The number in quare brackets is the time since startup in milliseconds.
```
    [1000] [CONNMGR] 165 0 0 0 0 0 0 --> 5
    software version MAC-QCA7005-1.1.0.730-04-20140815-CS 
    [1510] [ModemFinder] Number of modems: 1
    [1540] [CONNMGR] ConnectionLevel changed from 5 to 10.
```

after around 30s, the modem will go to sleep:
```
    [34090] [ModemFinder] Number of modems: 0
```

The amount of logged data can be configured with the parameter "logging" using the web interface (see below).

### Connection Manager Messages

The Connection Manager (short: CONNMGR) is a piece of software, which coordinates the different software layers, with the goal
to have a consistent state of all layers. It calculates a ConnectionLevel, based on the feedback of the layers and based
on timeout counters.
The log messages of the connection manager, e.g. `[CONNMGR] 165 0 0 0 0 0 0 --> 5` show the seven timeout counters and the resulting ConnectionLevel `--> 5`.
The meaning of the ConnectionLevels is
```
    CONNLEVEL_100_APPL_RUNNING Application messages are exchanged between the charger and clara.
    CONNLEVEL_80_TCP_RUNNING The TCP connection is established.
    CONNLEVEL_50_SDP_DONE The service discovery protocol has found a charger.
    CONNLEVEL_20_TWO_MODEMS_FOUND Two modems (the foccci QCA and the chargers modem) have formed a private network.
    CONNLEVEL_15_SLAC_ONGOING The SLAC sequence is ongoing.
    CONNLEVEL_10_ONE_MODEM_FOUND The STM controller on Foccci found the Focccis QCA modem.
    CONNLEVEL_5_ETH_LINK_PRESENT (No meaning. The physical SPI connection to the QCA modem is always present.)
```

The meaning of the seven timeout counters of the connection manager is explained here:
https://github.com/uhi22/ccs32clara/blob/main/ccs/connMgr.cpp#L44C14-L44C34

## How to setup the build chain?

There are two totally different tool chains for Clara: The old one, until November 2023, uses the STM32 CubeIDE. The new one comes with a classical make file, and offers full flexibility regarding which editor/IDE is used. The description below is valid for the new variant.

On Windows:

- Install git
- open a git command shell.
- clone the repository from github: `$ git clone https://github.com/uhi22/ccs32clara` or `$ git clone https://github.com/jsphuebner/ccs32clara`
- change into the cloned repository: `$ cd ccs32clara`
- The only external depedencies are libopencm3 and libopeninv.
- get the submodules `$ git submodule init` and `$ git submodule update`
- Mingw64 has mingw32-make.exe instead of make.exe in its bin folder. So instead of `make` type `$ mingw32-make`. If this does not succeed, follow up with the next points.
- Try whether the compiler is found: `$ arm-none-eabi-gcc`. You will need the arm-none-eabi toolchain: https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads
  If this is not yet intalled and you have already the STM CubeIDE installed, add the path to the compiler to your path variable, e.g.
C:\ST\STM32CubeIDE_1.12.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.10.3-2021.10.win32_1.0.200.202301161003\tools\bin Or, if the arm tool chain is not yet installed, then install it as explained in the mentioned link.
- Now the make should work: `$ mingw32-make`
- This leads to error due to missing header file nvic.h. If there is no nvic.h then libopencm3 needs to be compiled. nvic.h is generated by a python script. To install and compile libopencm3, `$ mingw32-make get-deps`
- Now the make should work: `$ mingw32-make`
- This should create a file stm32_ccs.hex.
- The Clara project is designed to live together with a bootloader, and this bootloader needs to be copied from here: https://github.com/jsphuebner/stm32-CANBootloader/releases/download/v1.0/stm32_canloader.hex
- To initially flash this on the foccci board using STLink, use `$ ST-LINK_CLI -P stm32_canloader.hex -P stm32_ccs.hex -V`
- After successful flashing, make a power-off-on cycle to start the application.
- As soon as the bootloader is on the board, you have three options to flash:
    - JTAG/SWD adapter (STLink, with the command line mentioned above)
    - can-updater.py script (todo: more details)
    - the esp32 web interface (see below)

## How to use Clara with the openinverter web interface?

The esp32-web-interface (from openinverter shop or https://github.com/jsphuebner/esp32-web-interface) is able to talk to Clara via CAN.
Both using 500kBaud. Select the NodeID 22 in the web interface to connect to Clara.

Using the web interface, it is possible e.g.
- to change parameters and store them into flash memory, so that they are surviving power-cycles.
- to view spot values and to graph them
- to add and change the CAN mapping, which defines which signals are transmitted and received in which CAN messages
