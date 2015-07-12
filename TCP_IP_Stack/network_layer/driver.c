#include "driver.h"
#include "enc28j60.h"
#include "stm32f4xx.h"

extern unsigned long sysTime;


/**
 * @brief  以太网卡初始化
 * @param  mac: 网卡MAC地址
 * @retval 网卡初始化成功标志，0: 成功
 */
mIPErr myTCPIP_driverInit(UINT8 *mac)
{
	enc28j60Init(mac);
	enc28j60PhyWrite(PHLCON,0x476);
	
	return mIP_OK;
}

/**
 * @brief  获取数据包
 * @param  buf: 装载数据buf的指针
 * @param  macLength: 最大读取数据量
 * @retval 读取的数据量，如果没有收到数据则返回0
 */
UINT32 myTCPIP_getPacket(UINT8 *buf, UINT32 maxLength)
{
	return enc28j60PacketReceive(maxLength, buf);
}

/**
 * @brief  发送数据包
 * @param  buf: 数据包指针
 * @param  length: 发送数据量
 * @retval 0: 发送成功
 */
mIPErr  myTCPIP_sendPacket(UINT8 *buf, UINT32 length)
{
	enc28j60PacketSend(length, buf);
	while(enc28j60Read(ECON1) & ECON1_TXRTS); /* 等待数据发送完毕，保证TCP能连续发送 */
	return mIP_OK;
}

/**
 * @brief  获取系统时间，系统时间变量由用户自行维护，单位为毫秒
 * @param  none
 * @retval 系统时间，单位为毫秒
 */
UINT32 myTCPIP_getTime(void)
{
	return sysTime;
}


