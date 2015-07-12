#ifndef __UDP_H
#define __UDP_H

#include "datatype.h"
#include "IP.h"

/**
 * @brief UDP首部 
 */
typedef struct UDPHeadStruct { 
	UINT16 srcPort;  /* 源端口号 */
	UINT16 dstPort;  /* 目标端口号 */
	UINT16 length;   /* 首部+数据的长度 */
	UINT16 checkSum; /* 校验和 */
} UDPHeadStruct;

/**
 * @brief UDP伪首部 
 */
typedef struct UDPPseudoHeadStruct {
	UINT8  srcIP[4]; /* 源IP地址 */
	UINT8  dstIP[4]; /* 目标IP地址 */
	UINT8  zero;     /* 全零字段 */
	UINT8  protocol; /* 协议，对应IP_PROTOCOL_UDP */
	UINT16 length;   /* UDP首部+数据长度 */
} UDPPseudoHeadStruct;


///* 当前服务器中UDP各种状态即参数 */
//typedef struct UDPStruct {
//	UINT8  number;    /* 用户建立的UDP的数量，如果没有建立则为0 */
//	UINT16 portTal[]; /* 端口表 */
//} UDPStruct;

void UDP_process(IPHeadStruct *ip, UINT32 *offset);
mIPErr UDP_send(UINT16 srcPort, UINT8 *dstIP, UINT16 dstPort, UINT8 *data, UINT32 dataLen);

#endif
