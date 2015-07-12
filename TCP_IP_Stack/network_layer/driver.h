#ifndef __DRIVER_H
#define __DRIVER_H

#include "datatype.h"

mIPErr myTCPIP_driverInit(UINT8 *mac);
UINT32 myTCPIP_getPacket(UINT8 *buf, UINT32 maxLength);
mIPErr  myTCPIP_sendPacket(UINT8 *buf, UINT32 length);
UINT32 myTCPIP_getTime(void);

#endif

