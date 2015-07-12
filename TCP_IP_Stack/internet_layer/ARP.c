#include "ARP.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量 
 */
extern myTCPIPStruct mIP;

/**
 * @brief 网卡的全局变量，能临时存储接收的MAC地址  
 */
extern ETHHeadStruct ETHHead;


/**
 * @brief  解析ARP首部 
 * @param  ptr: ARP首部结构体指针，接收解析后数据
 * @param  offset: buffer偏移量指针
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
static mIPErr ARP_analyzeHead(ARPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;

	ptr->hardwareType = MKUINT16BIG(buf);
	buf += 2;
	ptr->protocolType = MKUINT16BIG(buf);
	buf += 2;
	ptr->hardwareSize = *buf;
	buf += 1;
	ptr->protocolSize = *buf;
	buf += 1;
	ptr->opCode = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->senderMACAdd, buf, 6);
	buf += 6;
	bufCopy(ptr->senderIPAdd, buf, 4);
	buf += 4;
	bufCopy(ptr->targetMACAdd, buf, 6);
	buf += 6;
	bufCopy(ptr->targetIPAdd, buf, 4);
	buf += 4;
	*offset = buf - mIP.buf;

	return mIP_OK;
}

/**
 * @brief  通过ARP首部结构体制作ARP首部到buffer中
 * @param  ptr: ARP首部结构体指针
 * @param  offset: buffer偏移量指针
 * @retval none
 */
static void ARP_makeHead(ARPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	UINT16TOPTRBIG(buf, ptr->hardwareType);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->protocolType);
	buf += 2;
	*buf++ = ptr->hardwareSize;
	*buf++ = ptr->protocolSize;
	UINT16TOPTRBIG(buf, ptr->opCode);
	buf += 2;
	bufCopy(buf, ptr->senderMACAdd, 6);
	buf += 6;
	bufCopy(buf, ptr->senderIPAdd, 4);
	buf += 4;
	bufCopy(buf, ptr->targetMACAdd, 6);
	buf += 6;
	bufCopy(buf, ptr->targetIPAdd, 4);	
	buf += 4;
	*offset = buf - mIP.buf;
}

/**
 * @brief  ARP回应，发送本机mac地址
 * @param  ptr: ARP首部结构体
 * @retval none
 */
static void ARP_reply(ARPHeadStruct *ptr)
{
	ETHHeadStruct ethTmp;
	UINT32 offset = 0;
	
	/* 载入ETH头 */
	bufCopy(ethTmp.dstAdd, ptr->senderMACAdd, 6);
	bufCopy(ethTmp.srcAdd, mIP.mac, 6);
	ethTmp.type = PROTOCOL_TYPE_ARP;
	offset = 0;
	ETH_makeHead(&ethTmp, &offset);
	
	/* 载入ARP头 */
	ptr->opCode = ARP_OPCODE_REPLY; /* 响应操作 */
	bufCopy(ptr->targetMACAdd, ptr->senderMACAdd, 6);
	bufCopy(ptr->targetIPAdd, ptr->senderIPAdd, 4);
	bufCopy(ptr->senderMACAdd, mIP.mac, 6);
	bufCopy(ptr->senderIPAdd, mIP.ip, 4);
	ARP_makeHead(ptr, &offset);
	/* 发送ARP响应 */
	myTCPIP_sendPacket(mIP.buf, offset);
}     

/**
 * @brief  发送一个ARP查询，然后等待回应或超时
 * @param  dstIP: 目标IP地址
 * @param  macBuf: 用来接收目标IP对应的MAC地址
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
mIPErr ARP_request(UINT8 *dstIP, UINT8 *macBuf)
{
	ETHHeadStruct ethTmp;
	ARPHeadStruct arp;
	UINT32 offset = 0, len = 0, time = 0;
	UINT8  dstEthMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	UINT8  dstArpMac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	
	/* 载入ETH头 */
	bufCopy(ethTmp.dstAdd, dstEthMac, 6);
	bufCopy(ethTmp.srcAdd, mIP.mac, 6);
	ethTmp.type = PROTOCOL_TYPE_ARP;
	offset = 0;
	ETH_makeHead(&ethTmp, &offset);
	
	/* 载入ARP头 */
	arp.hardwareType = HARDWARE_TYPE_ETH;
	arp.protocolType = PROTOCOL_TYPE_IP;
	arp.hardwareSize = 6;
	arp.protocolSize = 4;
	arp.opCode       = ARP_OPCODE_REQ; /* 请求 */
	bufCopy(arp.senderMACAdd, mIP.mac, 6);
	bufCopy(arp.senderIPAdd, mIP.ip, 4);
	bufCopy(arp.targetMACAdd, dstArpMac, 6);
	bufCopy(arp.targetIPAdd, dstIP, 4);
	ARP_makeHead(&arp, &offset);	
    /* 发送ARP查询 */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	/* 等待主机的回应 */
	time = myTCPIP_getTime();
	while(1) {
		len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
		if(len == 0) {
			if(myTCPIP_getTime() - time > ARP_TIMEWAIT || 
			mIP.arpCache.arpUpdataTime > myTCPIP_getTime()) { /* 如果等待超时或定时器溢出，则认为没有此主机 */
				return mIP_NOACK;
			}
			continue;
		}
		
		offset = 0; /* 设置buf的偏移量为0 */
		ETH_analyzeHead(&ETHHead, &offset);
		if(ETHHead.type == PROTOCOL_TYPE_ARP) { /* ARP协议 */
			ARP_analyzeHead(&arp, &offset);
			if(arp.opCode == ARP_OPCODE_REPLY) { /* 如果是回应操作 */
				if(bufMatch(arp.targetIPAdd, mIP.ip, 4)) { /* 如果请求的IP与本机相匹配 */
					bufCopy(macBuf, arp.senderMACAdd, 6);
					break;
				}
			}
		}
	}

	return mIP_OK;
}

/**
 * @brief  检查ARP高速缓存中是否有dstIP对应的MAC地址，并检查是否需要更新缓存 
 * @param  dstIP: 目标IP
 * @param  macBuf: 接收目标IP对应的MAC地址
 * @retval 1: 查询并获取成功 0：目标IP不在缓存中
 */
UINT8 ARP_checkCache(UINT8 *dstIP, UINT8 *macBuf)
{
	UINT16 i;
	
	/* 检查是否缓存是否过期 */
	if(myTCPIP_getTime() - mIP.arpCache.arpUpdataTime > (ARP_CACHE_UPDATETIME * 1000) || 
	mIP.arpCache.arpUpdataTime > myTCPIP_getTime()) { /* 如果到了缓存更新时间或者定时器溢出，清空缓存 */
		mIP.arpCache.num = 0;
		mIP.arpCache.arpUpdataTime = myTCPIP_getTime(); /* 更新缓存时间 */
		return 0;
	}
	
	/* 检查缓存中是否有对应IP */
	for(i = 0; i < mIP.arpCache.num; i++) {
		if(bufMatch(dstIP, mIP.arpCache.ip[i], 4)) {
			bufCopy(macBuf, mIP.arpCache.mac[i], 6);
			return 1;
		}
	}
	
	return 0;
}

/**
 * @brief  添加一条IP和MAC到ARP高速缓存中,如果缓存已满则清空缓存并添加到起始位置 
 * @param  addIP: 添加的IP
 * @param  macBuf: 对应IP的MAC地址
 * @retval none
 */
void ARP_addCache(UINT8 *addIP, UINT8 *macBuf)
{
	if(mIP.arpCache.num >= ARP_CACHE_MAXNUM) { /* 如果超过了缓存最大允许条目，则设置缓存条目为0(清空缓存) */
		mIP.arpCache.num = 0;
		mIP.arpCache.arpUpdataTime = myTCPIP_getTime(); /* 更新缓存时间 */
	}
	/* 把刚查询的IP，MAC信息记录到缓存中 */
	bufCopy(mIP.arpCache.ip[mIP.arpCache.num], addIP, 4);
	bufCopy(mIP.arpCache.mac[mIP.arpCache.num], macBuf, 6);
	mIP.arpCache.num++;
}

/**
 * @brief  获取dstIP对应的MAC地址，优先在ARP高速缓存中查找，如果有，吧地址复制到macBuf里，如果没有，则发送一个ARP查询，再写入缓存 
 * @param  dstIP: 目标IP
 * @param  macBuf: 接收目标IP对应的MAC地址
 * @retval 0: 成功 1：不存在此主机 
 */
mIPErr ARP_getMacByIp(UINT8 *dstIP, UINT8 *macBuf)
{
	if(mIP.arpCache.num == 0 || !ARP_checkCache(dstIP, macBuf)) { /* 如果缓存中不存在数据或找不到对应IP, 则发送ARP查询 */
		if(ARP_request(dstIP, macBuf) == mIP_OK) {          
			ARP_addCache(dstIP, macBuf);
		}
		else { /* 如果局域网上不存在目标IP对应的主机，则失败 */
			return mIP_ERROR;
		}
	}
	
	return mIP_OK;
}

/**
 * @brief  ARP处理，主要用于自动回应客户端的ARP请求
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void ARP_process(UINT32 *offset)
{
	ARPHeadStruct arp;
	
	ARP_analyzeHead(&arp, offset);
    if(arp.opCode == ARP_OPCODE_REQ) { /* 如果是请求操作 */
		if(bufMatch(arp.targetIPAdd, mIP.ip, 4)) { /* 如果请求的IP与本机相匹配 */
			ARP_reply(&arp);
		}
	}
}

