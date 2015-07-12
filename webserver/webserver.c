#include "myTCPIP.h"
#include <stdio.h>
#include "http.h"
#include "ftp.h"

extern myTCPIPStruct mIP;

static UINT8 mymac[6]   = {0x54,0x55,0x56,0x57,0x58,0x59};
static UINT8 myip[4]    = {169,254,228,100};
static UINT8 netmask[4] = {255, 255, 0, 0};

static unsigned char buf[myICPIP_bufSize];
static unsigned char datBuf[myTCPIP_datBufSize];


void bufOutput(unsigned char *ptr, unsigned long size)
{
	unsigned long i;
	unsigned char tmp;
	unsigned char fnd = 0;
	
	
	for(i = 0; i < size; i++) {
		if(!(i & 0x0F)) {
			printf("\r\n");
			if(fnd) printf("%6x: ", i);
		}
		
		tmp = *ptr++;
		if(tmp <= 0x0F) 
			printf("0x0%X, ", tmp);
		else 
			printf("0x%X, ", tmp);
	}
}

void tcpCallBack(TCPInfoStruct *info)
{
	if(info->dstPort == TCP_PORT_HTTP) { /* HTTP */
		httpProcess(info);
	}
	else if(info->dstPort == TCP_PORT_FTP) { /* FTP */
		ftpProcess(info);
	}
}

void udpCallBack(UDPHeadStruct *udp, UINT8 *data, UINT32 dataLen)
{
	if(udp->dstPort == 10000) {
		printf("接收到了UDP数据：%s\r\n", data);
	}
}


void ftpDatTsmit(TCPInfoStruct *info);

/**
 * @brief 连接完全建立完成的回调函数 
 */
void coonEstabCallBack(TCPInfoStruct *info)
{
	if(info->dstPort == FTP_DATA_PORT) {
		ftpDatTsmit(info);
	}
}

TCPTaskStruct tcpTask;

void webserver(void)
{
	myTCPIP_init(mymac, myip, buf, datBuf);
	TCP_createTask(&tcpTask, tcpCallBack);
	while(1) {			
		myTCPIP_process();
	}
}
