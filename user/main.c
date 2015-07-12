#include "main.h"
#include "string.h"
#include "webserver.h"
#include "myTCPIP.h"

void ledInit(void) {
	GPIO_InitTypeDef  GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;            /* uart2的TX是PA2 */
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_OUT;        /* 复用模式 */
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;       /* 推挽输出 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
}
	

void Delay(unsigned long x)
{
	while(x--) {}
}

FATFS   fs;
FIL     file;
DIR     dir;
FILINFO fileInfo;

static TCHAR lfnameBuf[512];
int main(void)
{ 
	UINT8 res;

	ledInit();
	usartInit();
	spiInit();
	SysTick_Config(84000);
	printf(" \r\n");
	res = f_mount(&fs, "", 1);
	if(res == FR_OK) printf("FATFS init ok\r\n");
	else             printf("FATFS init fail\r\n");
	fileInfo.lfname = lfnameBuf; /* 长文件名buffer */
	fileInfo.lfsize = sizeof(lfnameBuf);
	webserver();
	while(1) {                                 
	}
}




