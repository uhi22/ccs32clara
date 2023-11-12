
/* global include file, which includes all other include files */
//#include "controllertype.h"
#include <string.h> /* memcpy */
//#include <stdio.h>
//#include <stdlib.h> /* abs */
//#include "main.h"

#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include "configuration.h"
#include "printf.h"
#include "connMgr.h"
#include "hardwareInterface.h"
#include "homeplug.h"
#include "ipv6.h"
#include "modemFinder.h"
#include "myHelpers.h"
#include "myScheduler.h"
#include "pevStateMachine.h"
#include "qca7000.h"
#include "tcp.h"
#include "udpChecksum.h"
#include "temperatures.h"
#include "pushbutton.h"
#include "xcp.h"
#include "my_math.h"
#include "anain.h"
#include "digio.h"

/* temporary stubs */
#define publishStatus(x, y)
#define log_v(x, ...)






