#include "myTCPIP.h"
#include "share.h"

/**
 * @brief 整个协议栈的全局变量  
 */
myTCPIPStruct mIP;

/**
 * @brief 网卡的全局变量，能临时存储接收的MAC地址  
 */
ETHHeadStruct ETHHead;


/**
 * @brief  初始化TCP/IP协议栈
 * @param  mac: mac地址指针
 * @param  ip: ip地址指针
 * @param  buf: buffer(缓存区)指针，以后数据的发送和接收都将使用这个地址，由mIP.buf维护
 * @param  datBuf: data buffer(数据缓存区)指针，用于临时读取数据交给上层，有mIP.datBuf维护
 * @retval 错误标志，0为成功
 */
mIPErr myTCPIP_init(UINT8 *mac, UINT8 *ip, UINT8 *buf, UINT8 *datBuf)
{
	bufCopy(mIP.mac, mac, 6);
	bufCopy(mIP.ip, ip, 4);
	mIP.buf = buf;
	mIP.datBuf = datBuf;
	mIP.identification = 0x0000;
	mIP.arpCache.num = 0;
	mIP.enFlag = 0;  /* 标志清零 */

	return myTCPIP_driverInit(mac);
}

/**
 * @brief  TCP/IP处理接口，没事就一直调用吧
 * @param  none
 * @retval none
 */
void myTCPIP_process(void)
{
    UINT32 len, offset = 0;
	
	len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
	if(len == 0) return;

	offset = 0; /* 设置buf的偏移量为0，以便后期处理 */
	ETH_analyzeHead(&ETHHead, &offset);
	bufCopy(mIP.macTmp, ETHHead.srcAdd, 6); /* 临时存储客户端MAC */
	
	if(ETHHead.type == PROTOCOL_TYPE_ARP) { /* ARP协议 */
		ARP_process(&offset);
		return;
	}
	if(ETHHead.type == PROTOCOL_TYPE_IP) { /* IP协议 */
		IP_process(&offset);
	}
}

