#ifndef __ICMP_H
#define __ICMP_H

#include "datatype.h"
#include "IP.h"

typedef struct ICMPHeadStruct {
	UINT8  type;           /* ICMP类型 */
	UINT8  code;           /* ICMP打码，和类型配合使用 */
	UINT16 checkSum;       /* 校验和 */
	UINT16 identification; /* 标识(针对询问报文) */
	UINT16 sequence;       /* 序列号(针对询问报文) */
	UINT8  *data;          /* 数据指针 */	
} ICMPHeadStruct;

void ICMP_process(IPHeadStruct *ip, UINT32 *offset);

#endif
