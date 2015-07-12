#ifndef __TCP_H
#define __TCP_H

#include "datatype.h"
#include "IP.h"
#include "mIPConfig.h"

/**
 * @brief TCP首部
 */
typedef struct TCPHeadStruct {
	UINT16 srcPort;  /* 源端口 */
	UINT16 dstPort;  /* 目的端口 */
	UINT32 squNum;   /* 序号,每个字节都会被编号 */
	UINT32 ackNum;   /* 确认号,表示期望接收的下一个报文段的序号 */
	UINT8  headLen;  /* 4bit， TCP首部长度，单位为4byte */

	/* headLen和flag之间有6bit的保留，在此占位 */

	UINT8  flag;     /* 6bit，分别代表URG, ACK, PSH, RST, SYN, FIN */
	UINT32 window;   /* 窗口，指出了对方允许发送的数据量,也就是自己的缓存能接收多大的数据量 */
	UINT16 checkSum; /* 校验和，和UDP一样，不仅数据也要校验，还要加入伪首部 */
	UINT16 urgPtr;   /* 紧急指针，表示本报文段中紧急数据的字节数,紧急数据之后为普通数据，即使串口为0也能发送数据 */
	
	UINT8  option[40];  /* 可选选项buffer指针，长度不固定，最大40byte */
	UINT32 MSS;      /* 最大报文长度，从option中分析出来的，默认536 */
} TCPHeadStruct;

/**
 * @brief TCP伪首部(实际上和UDP伪首部是一样的，除了协议为IP以外，为了代码结构简单点还是再声明一下) 
 */
typedef struct TCPPseudoHeadStruct {
	UINT8  srcIP[4]; /* 源IP地址 */
	UINT8  dstIP[4]; /* 目标IP地址 */
	UINT8  zero;     /* 全零字段 */
	UINT8  protocol; /* 协议，对应IP_PROTOCOL_TCP */
	UINT16 length;   /* TCP首部+数据长度 */
} TCPPseudoHeadStruct;

/**
 * @brief TCP连接信息，用户指定的回调函数使用
 */
typedef struct TCPInfoStruct {
	UINT8  srcIP[4];  /* 源IP */
	UINT16 srcPort;   /* 源端口 */
	UINT16 dstPort;   /* 目标端口 */
	UINT8  *data;     /* 数据指针 */
	UINT32 dataLen;   /* 数据长度 */
	UINT32 squNumRcv; /* 客户端发过来的序号 */
	UINT32 ackNumRcv; /* 客户端发过来的确认号 */
	UINT32 squNumSnd; /* 上次本机发送出去的序号 */
	UINT32 ackNumSnd; /* 上次本机发送出去的确认号 */
	UINT32 serverWnd; /* 本机剩余窗口大小 */
	UINT32 clientWnd; /* 客户端剩余窗口大小 */
	UINT32 clientMSS; /* 客户端MSS大小 */
} TCPInfoStruct;

/**
 * @brief TCP任务结构体，记录着所有连接的信息，由系统维护，但是空间需要空户申请
 */
typedef struct TCPTaskStrcut {
	UINT8         state[TCP_COON_MAXNUM];  /* 连接的状态 */
	TCPInfoStruct info[TCP_COON_MAXNUM];   /* 信息指针 */
} TCPTaskStruct;


void   TCP_process(IPHeadStruct *ip, UINT32 *offset);
mIPErr TCP_send(UINT8 *dstIP, UINT16 srcPort, UINT16 dstPort, UINT32 seqNum, UINT32 ackNum, UINT8 headLen, UINT8 flag, UINT16 window, UINT16 urgPtr, UINT8 *option, UINT8 *data, UINT32 dataLen);
void   TCP_createTask(TCPTaskStruct *ptr, void (*cbPtr)(TCPInfoStruct *));
mIPErr TCP_replyMultiStart(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_replyMulti(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_replyAndWaitAck(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_reply(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info);
mIPErr TCP_coonClose(TCPInfoStruct *info);
mIPErr TCP_waitAck(TCPInfoStruct *info, UINT8 mode);
mIPErr TCP_coonRelease(TCPInfoStruct *info);
mIPErr TCP_coonCloseAck(TCPInfoStruct *info);
UINT32 TCP_getInfoPos(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort);
void TCP_coonReset(void);

void coonEstabCallBack(TCPInfoStruct *info);
void coonSynCallBack(void);

#endif
