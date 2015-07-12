#ifndef __TCP_VIRTUALWINDOW_H
#define __TCP_VIRTUALWINDOW_H

#include "datatype.h"

mIPErr TCP_vndOpen(UINT8 *size, UINT32 *length, VND_METHOD method);
mIPErr TCP_vndWriteReady(UINT8 *dstName, VND_METHOD method);
mIPErr TCP_vndWrite(UINT8 *data, UINT32 dataLen, VND_METHOD method);
mIPErr TCP_vndGet(UINT8 *data, UINT32 dataLen, VND_METHOD method);
mIPErr TCP_vndMvPtr(UINT32 offset, VND_METHOD method);
mIPErr TCP_vndClose(VND_METHOD method);

#endif

