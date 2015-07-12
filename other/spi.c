#include "spi.h"
#include "stm32f4xx.h"

#define SPI_MODE_HARDWARE
#define SPI_DELAY() Delay(0xF)
void Delay(unsigned long x);

/**
 * @brief SPI引脚初始化(PA4-CS , PA5-CLK , PA6-MISO , PA7-MOSI)
 */
static void spiPinInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE); /* 使能GPIOA时钟 */
#ifdef SPI_MODE_HARDWARE                                 /* 硬件SPI */	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;        /* 复用模式 */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* 推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;            /* CS引脚以普通IO初始化 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;       
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	SPI_CS_H;
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);  /* 设置PA5引脚作为SPI1_CLK */
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_SPI1);  /* 设置PA6引脚作为SPI1_MISO */
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);  /* 设置PA7引脚作为SPI1_MOSI */

#else  /* 软件模拟SPI */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* 推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;            /* CS引脚以普通IO初始化 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;       
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	SPI_CS_H;
#endif
}

/**
 * @brief SPI初始化
 */
void spiInit(void)
{
#ifdef SPI_MODE_HARDWARE
	SPI_InitTypeDef spi;

	spiPinInit();  /* 初始化SPI1引脚 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE); /* 使能SPI1时钟 */
	
	spi.SPI_Direction = SPI_Direction_2Lines_FullDuplex; /* 双线双向全双工 */
	spi.SPI_Mode = SPI_Mode_Master;                      /* 主SPI */
	spi.SPI_DataSize = SPI_DataSize_8b;                  /* 8bit数据 */
	spi.SPI_CPOL = SPI_CPOL_Low;                         /* 时钟空闲时为低电平 */
	spi.SPI_CPHA = SPI_CPHA_1Edge;                       /* 前时钟沿采样，配合CPOL_L则为上升沿采样 */
	spi.SPI_NSS = SPI_NSS_Soft;                          /* 软件控制NSS */
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2; /* 2分频 */
	spi.SPI_FirstBit = SPI_FirstBit_MSB;                 /* 高位在前 */
	SPI_Init(SPI1, &spi);
	
	SPI_Cmd(SPI1, ENABLE);                               /* 使能SPI1 */

#else
	spiPinInit();  /* 初始化SPI1引脚 */
	SPI_CS_H;
	SPI_MOSI_H;
	SPI_CLK_L;
#endif
}

/**
 * @brief SPI发送并且接收一个字节,
 * 在spi.h中有两个宏：SPI_CS_H and SPI_CS_L,用来软件控制PA4
 * @param data: 即将发送的数据
 * @retval 接收的字节
 */
unsigned char spiSR(unsigned char data)
{
#ifdef SPI_MODE_HARDWARE
/*	
    注释的是使用库函数的发送接收代码，效率比直接操作寄存器低
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) != SET);
	SPI_I2S_SendData(SPI1, data);
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) != SET);
	return SPI_I2S_ReceiveData(SPI1);
 */
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = data;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE)) {}
    return SPI1->DR;
#else
	unsigned char i, tmp = 0;
	
	SPI_CLK_L;
	SPI_DELAY();
	for(i = 0; i < 8; i++) {
		if(data & (0x80 >> i)) SPI_MOSI_H;
		else                   SPI_MOSI_L;
		SPI_DELAY();
		tmp |= (SPI_MISO_IN) ? (0x80 >> i) : (0x00);
		SPI_DELAY();
		SPI_CLK_H;   /* 上升沿采样 */		
		SPI_DELAY(); 
		SPI_CLK_L;   /* 下降沿改变数据 */
	}
	
	return tmp;
#endif
}

/**
 * @brief SPI发一个字节,由于不管接收判断所以速度比spiSR快
 * 在spi.h中有两个宏：SPI_CS_H and SPI_CS_L,用来软件控制PA4
 * @param data: 即将发送的数据
 */
void spiSend(unsigned char data)
{
#ifdef SPI_MODE_HARDWARE
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = data;
#else

#endif
}


/**
 * @brief SPI接收一个字节,速度和spiSR一样
 * 在spi.h中有两个宏：SPI_CS_H and SPI_CS_L,用来软件控制PA4
 * @retval 接收到的数据
 */
unsigned char spiReceive(void)
{
#ifdef SPI_MODE_HARDWARE
	while(!(SPI1->SR & SPI_I2S_FLAG_TXE)) {}
	SPI1->DR = 0xFF;
	while(!(SPI1->SR & SPI_I2S_FLAG_RXNE)) {}
    return SPI1->DR;	
#else

#endif
}

