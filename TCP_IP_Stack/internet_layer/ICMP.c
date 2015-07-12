#include "ICMP.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量 
 */
extern myTCPIPStruct mIP;

/**
 * @brief  解析ICMP首部
 * @param  ptr: ICMP首部结构体指针，装载解析后的数据
 * @param  offset: buffer偏移量指针
 * @retval 0: 解析成功
 */
static mIPErr ICMP_analyzeHead(ICMPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->type = *buf++;
	ptr->code = *buf++;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	if(ptr->type == ICMP_TYPE_REQ) { /* 如果是询问报文就获取标识符和序列号 */
		ptr->identification = MKUINT16BIG(buf);
		buf += 2;
		ptr->sequence = MKUINT16BIG(buf);
		buf += 2;
	}
    ptr->data = buf;
	*offset = buf - mIP.buf;
	
	return mIP_OK;
}

/**
 * @brief  制作ICMP首部(实际上也包括数据)到buffer中，主要用于回应客户端的ping
 * @param  ptr: ICMP首部结构体指针
 * @param  offset: buffer偏移量指针
 * @param  data: 需发送的数据的指针
 * @param  dataLen: 需发送的数据长度
 * @param  ip: IP首部结构体,保存着客户端IP地址等信息
 * @retval none
 */
static void ICMP_makeHead(ICMPHeadStruct *ptr, UINT32 *offset, UINT8 *data, UINT32 dataLen, IPHeadStruct *ip)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT16 sum;
	UINT32 headLen = 8;
	
	/* 制作ICMP头 */
	*buf++ = ptr->type;
	*buf++ = ptr->code;
	UINT16TOPTRBIG(buf, 0x0000); /* 先吧校验和置为0 */
	buf += 2;
	if(ptr->type == ICMP_TYPE_REPLY) { /* 如果是回应 */
		headLen = 8;
		UINT16TOPTRBIG(buf, ptr->identification);
		buf += 2;
		UINT16TOPTRBIG(buf, ptr->sequence);
		buf += 2;
	}
	bufCopy(buf, data, dataLen); /* 复制数据 */
	sum = calcuCheckSum(mIP.buf + *offset, dataLen + headLen); /* 计算校验和 */
	UINT16TOPTRBIG(mIP.buf + *offset + 2, sum);
	
	
	*offset = buf - mIP.buf + dataLen;
} 

/**
 * @brief  ICMP处理(主要针对ping)
 * @param  ip: 客户端查询时的IP头部结构体
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void ICMP_process(IPHeadStruct *ip, UINT32 *offset)
{
	ICMPHeadStruct icmp;
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;
	
	ICMP_analyzeHead(&icmp, offset);
	if(icmp.type == ICMP_TYPE_REQ) { /* 如果类型为查询(一般是ping) */
	    sum = calcuCheckSum(buf, ip->totalLength - ip->headLength * 4); /* ICMP检验范围为头部+数据 */
		if(sum != 0) return; /* 校验不通过,丢弃 */

		bufCopy(mIP.datBuf, icmp.data, ip->totalLength - ip->headLength * 4 - 8); /* 读取数据 */
		/* 回应主机的REQ */
		*offset = 0;
		/* 写入ETH头 */
		ETH_makeHeadDefault(offset);
		/* 写入IP头 */
		IP_makeHeadDefault(ip->totalLength - ip->headLength * 4, mIP.identification++, 0, 0, IP_PROTOCOL_ICMP, offset);
        /* 写入ICMP头 */
		icmp.type = ICMP_TYPE_REPLY;
		icmp.code = ICMP_CODE_REPLY;
		ICMP_makeHead(&icmp, offset, mIP.datBuf, ip->totalLength - ip->headLength * 4 - 8, ip);
		/* 发送数据包 */
		myTCPIP_sendPacket(mIP.buf, *offset);
	}	
}

