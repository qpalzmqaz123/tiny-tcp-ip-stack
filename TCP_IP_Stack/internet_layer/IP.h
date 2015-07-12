#ifndef __IP_H
#define __IP_H

#include "datatype.h"

/**
 * @brief IP头结构体
 */
typedef struct IPHeadStruct { /* IP固定的20byte首部，不包括可变字段 */
	UINT8  version;        /* 4bit IP版本：一般为IPV4或IPV6 */
	UINT8  headLength;     /* 4bit 头部长度: 一般20byte */
	UINT8  diffServices;   /* 区分服务：一般不用 */
	UINT16 totalLength;    /* 数据和头的总长度 */
	UINT16 identification; /* 数据包标识 */
	UINT8  flag;           /* 3bit 标志 */
	UINT16 offset;         /* 13bit 片偏移(8个字节为一个偏移单位) */
	UINT8  timeToLive;     /* 生存时间 */
	UINT8  protocol;       /* 协议，包含ICMP，TCP等等 */
	UINT16 checkSum;       /* 头部校验和 */
	UINT8  srcAdd[4];      /* 源IP地址 */
	UINT8  dstAdd[4];      /* 目的IP地址 */	
} IPHeadStruct;


void IP_process(UINT32 *offset);
void IP_makeHead(IPHeadStruct *ptr, UINT32 *offset);
mIPErr IP_analyzeHead(IPHeadStruct *ptr, UINT32 *offset);
void IP_makeHeadDefault(UINT16 datLen, UINT16 identification, UINT8 flag, UINT16 fOffset, UINT8 protocol, UINT32 *offset);

#endif                    
