#ifndef __MYTCPIP_H
#define __MYTCPIP_H

#include "datatype.h"
#include "mIPConfig.h"
#include "driver.h"
#include "share.h"
#include "ETH.h"
#include "ARP.h"
#include "IP.h"
#include "ICMP.h"
#include "UDP.h"
#include "TCP.h"
#include "TCP_virtualWindow.h"



/* TCP */
#define TCP_FLAG_URG 0x20 /* 紧急URG，表示数据具有相当高的优先级，优先发送,需要和紧急指针配合使用 */
#define TCP_FLAG_ACK 0x10 /* 确认ACK，建立连接后所有报文段ACK必须置1 */
#define TCP_FLAG_PSH 0x08 /* 推送PSH，很少使用 */
#define TCP_FLAG_RST 0x04 /* 复位RST，置位表示TCP连接中出现严重错误，必须释放连接，再重新建立 */
#define TCP_FLAG_SYN 0x02 /* 同步SYN，若SYN = 1, ACK = 0,则表示请求建立连接，对方响应的时候SYN = 1, ACK = 1 */
#define TCP_FLAG_FIN 0x01 /* 终止FIN，释放连接 */

#define TCP_COON_CLOSED    0 /* TCP-已关闭 */
#define TCP_COON_SYNRCV    1 /* TCP-同步收到 */
#define TCP_COON_ESTAB     2 /* TCP-已建立连接 */
#define TCP_COON_CLOSEWAIT 3 /* TCP-关闭等待 */
#define TCP_COON_LASTACK   4 /* TCP-关闭的最后确认 */

#define TCP_PORT_HTTP   80 
#define TCP_PORT_FTP    21
#define TCP_PORT_TFTP   69
#define TCP_PORT_TELNET 23
 
/* IP */
#define IP_VER_IPV4      4
#define IP_VER_IPV6      6
#define IP_HEADLENGTH    5      /* 5*4 = 20 */
#define IP_FLAG_MF       0x01   /* 还有分片 */
#define IP_FLAG_DF       0x02   /* 不能分片 */
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_IGMP 2
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_EGP  8
#define IP_PROTOCOL_IGP  9
#define IP_PROTOCOL_UDP  17
#define IP_PROTOCOL_IPV6 41
#define IP_PROTOCOL_OSPF 89

/* ICMP */
#define ICMP_TYPE_REPLY 0  /* ICMP应答 */
#define ICMP_CODE_REPLY 0  /* ICMP应答代码 */
#define ICMP_TYPE_REQ   8  /* ICMP Echo请求(ping) */
#define ICMP_CODE_REQ   0  /* ICMP Echo请求代码(ping) */

/* ARP */
#define ARP_OPCODE_REQ   0x0001  /* 请求 */
#define ARP_OPCODE_REPLY 0x0002  /* 响应 */

/* ETH */
#define HARDWARE_TYPE_ETH  0x0001

/* protocol type */
#define PROTOCOL_TYPE_ARP  0x0806
#define PROTOCOL_TYPE_IP   0x0800
#define PROTOCOL_TYPE_IPV6 0x86DD

/* enFlag */
#define ENFLAG_TCP 0x01 /* 使能TCP */
#define ENFLAG_UDP 0x02 /* 使能UDP */

/* ARP缓存结构体 */
typedef struct ARPCacheStruct {
	UINT16 num;                       /* 目前缓存区存储条目的数量，初始为0 */
	UINT8  mac[ARP_CACHE_MAXNUM][6];  /* 缓存的MAC地址 */
	UINT8  ip[ARP_CACHE_MAXNUM][4];   /* 缓存的IP地址，IP中单元和MAC中单元是一一对应的 */	
	UINT32 arpUpdataTime;             /* ARP更新时间 */
} ARPCacheStruct;

/* 协议栈结构体 */
typedef struct myTCPIPStruct {
	UINT8  *buf;             /* 主BUF，存储从网络接收的数据包 */
	UINT8  *datBuf;          /* 数据buf，存储临时数据 */
	UINT8  mac[6];           /* 本机MAC地址 */
	UINT8  macTmp[6];        /* 存储上次接收到的客户端MAC */
	UINT8  ip[4];            /* 本机IP地址 */
	UINT8  ipTmp[4];         /* 存储上次接收到的客户端IP */
	UINT16 identification;   /* 系统维护的标志，数据不需分片时发送一次IP加1 */
	UINT8  enFlag;           /* 使能标志，表明是否使用的TCP或者UDP */
	
	ARPCacheStruct arpCache; /* ARP高速缓存 */
	TCPTaskStruct *tcpTask;   /* TCP任务指针 */
	void (*tcpCb)(TCPInfoStruct *); /* TCP回调函数指针 */
} myTCPIPStruct;


mIPErr myTCPIP_init(UINT8 *mac, UINT8 *ip, UINT8 *buf, UINT8 *datBuf);
void   myTCPIP_process(void);

#endif

