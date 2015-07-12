#ifndef __ARP_H
#define __ARP_H

#include "datatype.h"

typedef struct ARPHeadStruct {
	UINT16 hardwareType;    /* 硬件类型 */
	UINT16 protocolType;    /* 高层的协议类型 */
	UINT8  hardwareSize;    /* 硬件地址长度，一般为6字节 */
	UINT8  protocolSize;    /* 高层协议长度 */
	UINT16 opCode;          /* 操作类型：request:0x0001  reply:0x0002 */
	UINT8  senderMACAdd[6]; /* 发送方MAC地址 */
	UINT8  senderIPAdd[4];  /* 发送方IP地址 */
	UINT8  targetMACAdd[6]; /* 目标MAC地址 */
	UINT8  targetIPAdd[4];  /* 目标IP地址 */
} ARPHeadStruct;

void ARP_process(UINT32 *offset);
mIPErr ARP_request(UINT8 *dstIP, UINT8 *macBuf);
UINT8 ARP_checkCache(UINT8 *dstIP, UINT8 *macBuf);
void ARP_addCache(UINT8 *addIP, UINT8 *macBuf);
mIPErr ARP_getMacByIp(UINT8 *dstIP, UINT8 *macBuf);

#endif
