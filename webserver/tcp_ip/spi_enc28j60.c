/******************** (C) COPYRIGHT 2011 迷你嵌入式开发工作室 ********************
 * 文件名  ：spi.c
 * 描述    ：ENC28J60(以太网芯片) SPI接口应用函数库
 *          
 * 实验平台：野火STM32开发板
 * 硬件连接： ------------------------------------
 *           |PB13         ：ENC28J60-INT (没用到)|
 *           |PA6-SPI1-MISO：ENC28J60-SO          |
 *           |PA7-SPI1-MOSI：ENC28J60-SI          |
 *           |PA5-SPI1-SCK ：ENC28J60-SCK         |
 *           |PA4-SPI1-NSS ：ENC28J60-CS          |
 *           |PE1          ：ENC28J60-RST (没用)  |
 *            ------------------------------------
 * 库版本  ：ST3.0.0
 * 作者    ：fire  QQ: 313303034
 * 博客    ：firestm32.blog.chinaunix.net 
**********************************************************************************/
#include "spi_enc28j60.h"
#include "spi.h"

/*
 * 函数名：SPI1_Init
 * 描述  ：ENC28J60 SPI 接口初始化
 * 输入  ：无 
 * 输出  ：无
 * 返回  ：无
 */																						  
void SPI_Enc28j60_Init(void)
{
	spiInit();
}

/*
 * 函数名：SPI1_ReadWrite
 * 描述  ：SPI1读写一字节数据
 * 输入  ： 
 * 输出  ：
 * 返回  ：
 */

unsigned char	SPI1_ReadWrite(unsigned char writedat)
{
	return spiSR(writedat);
}

