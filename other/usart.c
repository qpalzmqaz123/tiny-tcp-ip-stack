#include "usart.h"
#include <stdio.h>
#include "stm32f4xx.h"

/**
 * @brief 初始化uartIO
 */
static void usartPinInit(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;            /* uart2的TX是PA2 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;        /* 复用模式 */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* 推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);  /* 设置PA2引脚作为usart2使用 */
}

/**
 * @brief 初始化串口
 */
void usartInit(void)
{
	USART_InitTypeDef usart;
	
	usartPinInit();  /* 初始化uart2对应的TX引脚 */
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);              /* 使能uart2的时钟 */
	usart.USART_BaudRate = 115200;                                      /* 波特率为115200 */
	usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;   /* 失能硬件流控制 */
	usart.USART_Mode = USART_Mode_Tx;                                   /* 仅发送 */
	usart.USART_Parity = USART_Parity_No;                               /* 失能校验 */
	usart.USART_StopBits = USART_StopBits_1;                            /* 1位停止位 */
	usart.USART_WordLength = USART_WordLength_8b;                       /* 8byte长度 */
	
	USART_Init(USART2, &usart);
	USART_Cmd(USART2, ENABLE);   /* 使能usart2 */
	
}

/**
 * @brief 发送一个字节
 * @param data: 需要发送的数据
 */
static void usartSend(unsigned char data)
{
	USART_SendData(USART2, data);    /* 发送数据 */
	while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) != SET);  /* 等待发送完成 */
}

/**
 * @brief printf函数调用
 */
int fputc(int c, FILE *f)
{
  	usartSend(c);
  	return c;
}
