#include "UDP.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量
 */
extern myTCPIPStruct mIP;


/**
 * @brief  解析UDP头
 * @param  ptr: UDP首部结构体指针，用来存放解析后数据
 * @param  offset: offset: buffer中偏移量指针
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
static mIPErr UDP_analyzeHead(UDPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	ptr->srcPort  = MKUINT16BIG(buf);
	buf += 2;
	ptr->dstPort  = MKUINT16BIG(buf);
	buf += 2;
	ptr->length   = MKUINT16BIG(buf);
	buf += 2;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	
	*offset = buf - mIP.buf;
	
	return mIP_OK;
}

/**
 * @brief  通过UDP伪首部结构体制作UDP伪首部到buf中
 * @param  psPtr: UDP伪首部结构体
 * @param  buf: 存放解析后伪首部的buffer
 * @retval none
 */
static void UDP_makePsHead(UDPPseudoHeadStruct *psPtr, UINT8 *buf)
{
	bufCopy(buf, psPtr->srcIP, 4);
	buf += 4;
	bufCopy(buf, psPtr->dstIP, 4);
	buf += 4;
	*buf++ = psPtr->zero;                
	*buf++ = psPtr->protocol;
	UINT16TOPTRBIG(buf, psPtr->length);
}

/**
 * @brief  通过UDP首部结构体制作UDP首部到buffer中(实际上除了文件首部还有数据),伪首部不放入buffer，但要参与校验和的计算
 * @param  ptr: UDP首部指针
 * @param  psPtr: UDP伪首部指针
 * @param  offset: buffer偏移量指针
 * @param  data: 传送的数据指针
 * @param  dataLen: 传送的数据大小
 * @retval none
 */
static void UDP_makeHead(UDPHeadStruct *ptr, UDPPseudoHeadStruct *psPtr, UINT32 *offset, UINT8 *data, UINT32 dataLen)
{
	UINT8  *buf = mIP.buf + *offset;
	UINT8  psBuf[12];                /* 12字节的伪首部 */
	UINT16 sum;                      /* 校验和 */
	
	UDP_makePsHead(psPtr, psBuf);    /* 生成UDP伪首部,存入psBuf */
	
	/* 生成UDP首部 */
	UINT16TOPTRBIG(buf, ptr->srcPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->dstPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->length);
	buf += 2;
	UINT16TOPTRBIG(buf , 0x0000); /* 首先把校验和设置为0 */	
	buf += 2;
	bufCopy(buf, data, dataLen);  /* 装载数据 */
	buf += dataLen;
	
    sum = calcuCheckSum2Buf(psBuf, 12, mIP.buf + *offset, ptr->length); /* 生成校验和 */
	UINT16TOPTRBIG(mIP.buf + *offset + 6, sum);     /* 写入校验和 */
	
	*offset = buf - mIP.buf;
}

/**
 * @brief  使用UDP协议发送数据
 * @param  srcPort: 源端口，即本机发送UDP的端口
 * @param  dstIP: 目标IP
 * @param  dstPort: 目标端口
 * @param  data: 发送的数据的指针
 * @param  dataLen: 发送的数据的长度
 * @retval mIP_OK: 发送成功, mIP_ERROR: 发送失败
 */
mIPErr UDP_send(UINT16 srcPort, UINT8 *dstIP, UINT16 dstPort, UINT8 *data, UINT32 dataLen)
{
	UDPHeadStruct udp;
	UDPPseudoHeadStruct udpPs;
	ETHHeadStruct eth;
	UINT32 offset;
	UINT8 macTmp[6];
	
	/* 制作UDP首部 */
	udp.srcPort  = srcPort;
	udp.dstPort  = dstPort;
	udp.length   = dataLen + 8;
	udp.checkSum = 0x0000;
	
	/* 制作UDP伪首部 */
	bufCopy(udpPs.srcIP, mIP.ip, 4);
	bufCopy(udpPs.dstIP, dstIP, 4);
	udpPs.zero = 0x00;
	udpPs.protocol = IP_PROTOCOL_UDP;
	udpPs.length = dataLen + 8;
	
	/* 发送数据前获取IP对应的MAC地址，如果获取不到返回ERROR */
	if(ARP_getMacByIp(mIP.ipTmp, macTmp) != mIP_OK) return mIP_NOACK;
	
	/* 偏移量归零，开始发送数据 */
	offset = 0;
	/* 写入ETH头 */
	bufCopy(eth.dstAdd, macTmp, 6);
	bufCopy(eth.srcAdd, mIP.mac, 6);
	eth.type = PROTOCOL_TYPE_IP;
	ETH_makeHead(&eth, &offset);
	/* 写入IP头 */
	IP_makeHeadDefault(udp.length, mIP.identification++, 0, 0, IP_PROTOCOL_UDP, &offset);
	/* 写入UDP头 */
	UDP_makeHead(&udp, &udpPs, &offset, data, dataLen);
	/* 发送数据 */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	return mIP_OK;
}

/**
 * @brief  UDP处理
 * @param  ip: 承载UDP的IP首部指针，主要用来计算UDP数据大小，校验等
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void UDP_process(IPHeadStruct *ip, UINT32 *offset)
{
	UDPHeadStruct udp;
	UDPPseudoHeadStruct udpPs;
	UINT8 psBuf[12];             /* 12字节的伪首部 */
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;

	/* 设置伪首部 */
	bufCopy(udpPs.srcIP, ip->srcAdd, 4);
	bufCopy(udpPs.dstIP, ip->dstAdd, 4);
	udpPs.zero = 0x00;
	udpPs.protocol = ip->protocol;
	udpPs.length = ip->totalLength - ip->headLength * 4;	
	UDP_makePsHead(&udpPs, psBuf); /* 生成UDP伪首部,存入psBuf */
	
	sum = calcuCheckSum2Buf(psBuf, 12, buf, ip->totalLength - ip->headLength * 4); /* 计算校验和 */	
	if(sum != 0) return; /* 如果校验和不为0，丢包 */ 

	UDP_analyzeHead(&udp, offset); /* 分析UDP首部 */

//////////////////////
	bufCopy(mIP.datBuf, mIP.buf + *offset, udp.length - 8);
	udpCallBack(&udp, mIP.datBuf, udp.length - 8);
}

