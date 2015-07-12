#include "IP.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量
 */
extern myTCPIPStruct mIP;

/**
 * @brief  解析IP头
 * @param  ptr: IP头结构体指针，用来存放解析后的数据 
 * @param  offset: 偏移量指针，表示IP头信息在buffer中的偏移量
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
mIPErr IP_analyzeHead(IPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->version = (UINT8)(*buf >> 4);
	ptr->headLength = *buf++ & 0x0F;
    ptr->diffServices = *buf++;
	ptr->totalLength = MKUINT16BIG(buf);
	buf += 2;
	ptr->identification = MKUINT16BIG(buf);
	buf += 2;
	ptr->flag = *buf >> 5;
	ptr->offset = ((UINT16)(*buf & 0x1F) << 8) | (UINT16)(*(buf + 1));
	buf += 2;
	ptr->timeToLive = *buf++;
	ptr->protocol = *buf++;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->srcAdd, buf, 4);
	buf += 4;
	bufCopy(ptr->dstAdd, buf, 4);
	buf += 4;
    *offset += ptr->headLength * 4;
	
	return mIP_OK;
}

/**
 * @brief  通过IP头结构体制作IP头到buffer中
 * @param  ptr: IP头结构体指针
 * @param  offset: buffer中偏移量指针 
 * @retval none
 */
void IP_makeHead(IPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT16 sum;
	
	*buf++ = (ptr->version << 4) | ptr->headLength;
	*buf++ = ptr->diffServices;
	UINT16TOPTRBIG(buf, ptr->totalLength);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->identification);
	buf += 2;
	UINT16TOPTRBIG( buf, (((UINT16)ptr->flag << 5) | (ptr->offset & 0x1FFF)) );
	buf += 2;
	*buf++ = ptr->timeToLive;
	*buf++ = ptr->protocol;
	UINT16TOPTRBIG(buf, 0x0000);  /* 首部校验和先设置为0 */
	buf += 2;
	bufCopy(buf, ptr->srcAdd, 4);
	buf += 4;
	bufCopy(buf, ptr->dstAdd, 4);
	buf += 4;
	
	sum = calcuCheckSum(mIP.buf + *offset, ptr->headLength * 4); /* 计算首部校验和 */
	UINT16TOPTRBIG((mIP.buf + *offset + 10), sum);

	*offset = buf - mIP.buf;
}

/**
 * @brief  制作默认IP头，默认版本号为IPv4，首部大小为20byte，源IP为本机IP，生存时间为128
 * @param  totalLength: 数据的大小 
 * @param  identification: 标识符，维护在mIP.identification中，在分片发送时用同一标识符以便区分
 * @param  flag: 标志，只有3位有效，0x00或0x20表示后面没有分片， 0x01表示后面还有分片
 * @param  fOffset: 偏移量，表示不同分片在数据中的偏移量，只有13位有效，以8byte为单位，这样每个分片的长度位8字节的整数倍
 * @param  protocol: 高层使用的协议
 * @param  offset: buffer中偏移量指针
 * @retval none
 */
void IP_makeHeadDefault(UINT16 datLen, UINT16 identification, UINT8 flag, UINT16 fOffset, UINT8 protocol, UINT32 *offset)
{
	IPHeadStruct ip;
	ip.version = 4;
	ip.headLength = 5;
	ip.diffServices = 0x00;
	ip.totalLength = datLen + 20;
	ip.identification = identification;
	ip.flag = flag;
	ip.offset = fOffset;
	ip.timeToLive = 0x80;
	ip.protocol = protocol;
	ip.checkSum = 0x0000;
	bufCopy(ip.srcAdd, mIP.ip, 4);
	bufCopy(ip.dstAdd, mIP.ipTmp, 4);
	IP_makeHead(&ip, offset);         
}

/**
 * @brief  IP处理
 * @param  offset: buffer中偏移量指针 
 * @retval none
 */
void IP_process(UINT32 *offset)
{
	IPHeadStruct ip;
	UINT16 sum;
	
	sum = calcuCheckSum(mIP.buf + *offset, 20); /* IP只检验头部 */
	if(sum != 0) return; /* 校验不通过,丢弃此包 */

	IP_analyzeHead(&ip, offset); /* 解析IP固定的20byte头 */
	if(ARP_checkCache(ip.srcAdd, mIP.macTmp) == 0) { /* 检查请求的客户端的IP与MAC是否在ARP缓存中，如果不在则追加 */
		ARP_addCache(ip.srcAdd, mIP.macTmp);
	}
	
	if(bufMatch(ip.dstAdd, mIP.ip, 4) == 0) { /* 如果目标地址不是本机IP，丢弃 */
		return;
	}
	bufCopy(mIP.ipTmp, ip.srcAdd, 4); /* 临时存储客户端IP */ 
	
	if(ip.protocol == IP_PROTOCOL_ICMP) { /* 如果是ICMP协议(一般情况是ping本机) */
		ICMP_process(&ip, offset);
	}
	if((ip.protocol == IP_PROTOCOL_UDP) && (mIP.enFlag & ENFLAG_UDP)) {  /* UDP协议 */
		UDP_process(&ip, offset);
	}
	if((ip.protocol == IP_PROTOCOL_TCP) && (mIP.enFlag & ENFLAG_TCP)) {  /* TCP协议 */
		TCP_process(&ip, offset);
	}
}

