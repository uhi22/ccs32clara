
/* global include file, which includes all other include files */
#include "controllertype.h"
#include <string.h> /* memcpy */
#include <stdio.h>
#include <stdlib.h> /* abs */
#include "main.h"

#include "configuration.h"
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
#include "canbus.h"
#include "myAdc.h"
#include "temperatures.h"

/* temporary stubs */
#define publishStatus(x, y)
#define log_v(x, ...)

extern uint16_t debugvalue;





