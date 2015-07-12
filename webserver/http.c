#include "http.h"
#include "http_ack.h"
#include "myTCPIP.h"
#include <string.h>
#include "ff.h"
#include <stdio.h>
#include "stm32f4xx.h"
                             
#define CHECK 0
#define WAIT  1							 

extern FATFS fs;
extern FIL  file;

							 
static UINT8 HTTP_STATUS_BUF[200];
static UINT8 sndBuf[1550];
static UINT8 urlBuf[_MAX_LFN];
static UINT8 nameBuf[_MAX_LFN];

unsigned int decodeJson(char *jsonStr, char *attr, char *data);

/** 
 * @brief 虚拟主机设置
 */
#define VirtualHostNum 2 /* 虚拟主机数量 */
static const char VirtualHost[VirtualHostNum * 2][255] = { /* 虚拟主机设置，格式为"主机名","路径名" */
	"stm32", "/stm32",                   
	"server", "/server"
};


mIPErr TCP_sendDataFast(UINT32 mss, UINT32 size, TCPInfoStruct *info)
{
	mIPErr ack;
	UINT32 tmpSqu, tmpAck, fstSqu, fstAck, tmp;
	UINT32 time;                     
	UINT32  p1 = 0, p2 = 0, p3 = 0;  /* 窗口的三个指针,表示相对于传输内容首部的偏移量 */
	                                 /* p1表示已经确认对方收到了多少字节，p2表示已发送但未收到确认，p3表示窗口首部 */
	fstSqu = info->squNumRcv;        
	fstAck = info->ackNumRcv;
	tmpSqu = info->squNumRcv;        
	tmpAck = info->ackNumRcv;        /* 临时存储客户端的squ和ack */
	
	if(size <= info->clientWnd) {    /* 如果发送的数据大小小于或等于对方的窗口大小,那么p3直接移动到文件尾部 */
		p3 = size - 1;
	}
	else {
		p3 = info->clientWnd - 1;    
	}
	TCP_vndMvPtr(0, METHOD_FATFS); 

	if(size < mss) { /* 如果数据相当少，那么直接发送并等待ack就行了 */
		TCP_vndGet(sndBuf, size, METHOD_FATFS);
		TCP_replyAndWaitAck(sndBuf, size, info);
		TCP_coonClose(info);
	}
	else { /* 如果数据很多就需要模拟滑动窗口的方式发送数据 */
		time = myTCPIP_getTime();
		while(1) {
			tmp = ((p3 - p2 + 1) > mss) ? (mss) : (p3 - p2 + 1);  /* 计算即将发送的大小，并限制最大为mss */
			if(p2 > p3 || info->clientWnd == 0) tmp = 0;          
			
			TCP_vndMvPtr(p2, METHOD_FATFS);                       /* 移动指针到P2位置(即将发送的数据所在位置) */
			TCP_vndGet(sndBuf, tmp, METHOD_FATFS);		          /* 获取即将发送的数据 */
			
			
			
			if(tmp != 0) {			
				TCP_send(info->srcIP, /* 目标IP为源IP */
				 info->dstPort,       /* 源端口为客户端发送的目标端口 */
				 info->srcPort,       /* 目标端口为客户端的端口 */
				 fstAck + p2,         /* 序号，前面已经处理好了 */
				 tmpSqu,              /* 确认号，前面已经处理好了 */
				 20 / 4,              /* TCP首部长度：固有的20字节+MSS */
				 TCP_FLAG_ACK,        /* 响应 */
				 TCP_WINDOW_MAXSIZE,  /* 这里以后需要修改 */
				 0,                   /* 紧急指针 */
				 sndBuf,              /* MSS */
				 sndBuf,              /* 数据指针 */
				 tmp                  /* 数据长度 */
				 );
			}		
			p2 += tmp;		                       /* tmp大小数据发送完毕 */
		
			ack = TCP_waitAck(info, CHECK);        /* 检查客户端是否回应 */
			if(ack == mIP_OK) {                                  
				time = myTCPIP_getTime();          /* 响应后更新时间 */
				tmpSqu = info->squNumRcv;          
				tmpAck = info->ackNumRcv;
				p1 = tmpAck - fstAck;
				p3 = p1 + info->clientWnd;
				if(p3 > size - 1) p3 = size - 1;
                /**/
				if(p2 == p3 + 1) p2 = p1;
			}
			else if(ack == mIP_RST) {
				return mIP_RST;
			}
			else if(ack == mIP_FIN) {
				return mIP_FIN;
			}
			if((myTCPIP_getTime() - time > TCP_RETRY_TIME) || (time > myTCPIP_getTime())) {
				return mIP_NOACK; /* 如果超时没有响应的话，那么返回NOACK */
			}
		}
	}
	
	return mIP_OK;
}

void htttpAnalyzeUrl(UINT8 *url, UINT8 *fileName)
{
	UINT32 i;
	UINT32 len;

	if(strPos(url, strlen((char *)url), (UINT8 *)"?", 1))
		len = strPos(url, strlen((char *)url), (UINT8 *)"?", 1) - 1;
	else
		len = strlen((char *)url);

	bufCopy(fileName, url, len);
	*(fileName + len) = '\0';
	
	for(i = len - 1; i > 0; i--) {
		if(*(fileName + i) == '.') {
			break;
		}
	}
	
	if(i == 0) { /* 如果没有后缀名，则有可能是文件，也有可能是目录，不过一般都是目录 */
		if(bufMatch(url, (UINT8 *)"/", 2)) strcat((char *)fileName, "index.html"); 
		else                               strcat((char *)fileName, "index.html");
	}
}

void httpMakeReply(UINT16 ackNum, UINT8 *fileName, UINT32 cntSize)
{
	UINT32 i;
	
	for(i = strlen((char *)fileName); i > 0; i--) {
		if(*(fileName + i) == '.') {
			break;
		}
	}
	
	switch(ackNum) {
		case 200: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 200 OK\r\n"); break;
		case 400: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 400 Bad Request\r\n"); break;
		case 404: sprintf((char *)HTTP_STATUS_BUF, "HTTP/1.0 404 Not Found\r\n"); break;
		default: break; 
	}
	
	if(bufMatch(fileName + i + 1, (UINT8 *)"html", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: text/html\r\n");
	}
	else if(bufMatch(fileName + i + 1, (UINT8 *)"mp3", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: audio/mpeg\r\n");
	}
	else if(bufMatch(fileName + i + 1, (UINT8 *)"mp4", 4)) {
		strcat((char *)HTTP_STATUS_BUF, "Content-Type: video/mp4\r\n");
	}
	
	sprintf((char *)HTTP_STATUS_BUF + strlen((char *)HTTP_STATUS_BUF),
	        "Content-Length: %u\r\n\r\n", (unsigned int)cntSize);
}

void httpGetHtml(TCPInfoStruct *info)
{
	mIPErr res;
	UINT32 mss, size;
	
	htttpAnalyzeUrl(urlBuf, nameBuf); 
	printf("fileName: %s\r\n", nameBuf);
		
	mss = (TCP_OPTION_MSS > info->clientMSS) ? (info->clientMSS) : (TCP_OPTION_MSS); /* 计算一次能发出的最大数据量 */
	res = TCP_vndOpen(nameBuf, &size, METHOD_FATFS); /* 首先判断这请求的页面是否存在 */

	if(res == mIP_OK) { /* 如果存在这样的文件 */

		httpMakeReply(200, nameBuf, size);
		res = TCP_replyAndWaitAck((UINT8 *)HTTP_STATUS_BUF, strlen((char *)HTTP_STATUS_BUF), info); /* 首先返回200，等待响应，主要是为了方便之后快速传文件的操作 */
//		if(mss > 1024) mss = 1024; /* 针对SD卡的优化 */
		res = TCP_sendDataFast(mss, size, info);	
		
		if(res == mIP_NOACK) { /* 如果对方不响应则主动关闭连接 */
			printf("NOACK\r\n");
			TCP_coonClose(info);
		}
		else if(res == mIP_FIN) { /* 如果对方主动关闭连接 */
			printf("FIN\r\n");
			TCP_coonCloseAck(info);
		}
		else if(res == mIP_RST) { /* 如果对方发送RST则立马释放连接资源 */
			printf("RST\r\n");
			TCP_coonRelease(info);
		}
	}
	else { /* 不然返回404错误 */
		httpMakeReply(404, nameBuf, strlen(HTTP_ACK_404));
		TCP_replyAndWaitAck((UINT8 *)HTTP_STATUS_BUF, strlen((char *)HTTP_STATUS_BUF), info);
		TCP_replyAndWaitAck((UINT8 *)HTTP_ACK_404, strlen(HTTP_ACK_404), info);
		TCP_coonClose(info);
	}
}

void httpGetProcess(TCPInfoStruct *info)
{
	httpGetHtml(info);
}

void httpCallBack(char *url) {
	char dataBuf[20], flag = 0;
	char ledState[3];
	
	flag = decodeJson(url, "led0", dataBuf);
	if(flag == 0) return;
	printf("dataBuf: %s\r\n", dataBuf);
	ledState[0] = (dataBuf[0] == '0') ? (0) : (1);
	
	flag = decodeJson(url, "led1", dataBuf);
	if(flag == 0) return;
	ledState[1] = (dataBuf[0] == '0') ? (0) : (1);
	
	flag = decodeJson(url, "led2", dataBuf);
	if(flag == 0) return;
	ledState[2] = (dataBuf[0] == '0') ? (0) : (1);
	
	printf("flag0: %d, flag1: %d, flag2: %d\r\n", ledState[0], ledState[1], ledState[2]);
	if(ledState[0]) GPIO_SetBits(GPIOC, GPIO_Pin_0);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_0);
	if(ledState[1]) GPIO_SetBits(GPIOC, GPIO_Pin_1);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_1);
	if(ledState[2]) GPIO_SetBits(GPIOC, GPIO_Pin_2);
	else            GPIO_ResetBits(GPIOC, GPIO_Pin_2);
	
}

void httpProcess(TCPInfoStruct *info)
{
	UINT32 pathLen, i;
	UINT8 *p = info->data;
	
//	printf("info->data: %s\r\n", info->data);
	if(bufMatch(info->data, (UINT8 *)"GET", 3)) { /* 如果操作是GET */
		while(1) {                                /* 跳转到下一行 */
			if(*p == '\r' && *(p + 1) == '\n') {
				p += 2;
				break;
			}
			p++;
		}
		if(bufMatch(p, (UINT8 *)"Host", 4)) {     /* 解析host */
			p += 6;
			for(i = 0; i < VirtualHostNum; i++) { /* 检查host是否与虚拟主机对应 */
				if(bufMatch(p, (UINT8 *)VirtualHost[i * 2], strlen(VirtualHost[i * 2]))) {
					strcpy((char *)urlBuf, VirtualHost[i * 2 + 1]);
					break;
				}
			}
		}
		pathLen = strPos(info->data + 4, _MAX_LFN, (UINT8 *)" ", 1) - 1;
		if(pathLen == 0) return;
		*(info->data + 4 + pathLen) = '\0';
		strcat((char *)urlBuf, (char *)(info->data + 4));
printf("jfdlsaf: %s\r\n", (char *)(info->data + 4));
httpCallBack((char *)(info->data + 4));
		httpGetProcess(info);
	}
	
	TCP_coonReset(); /* 重置所有连接 */
}


/**
 * @brief  解析Url和Post中的json,一维
 * @param  jsonStr: json转换后的字符串
 * @param  attr: 需要查找的元素名称
 * @param  data: 属性对应的数据(字符串)
 * @retval 0: 匹配失败 !0: 匹配成功
 */
unsigned int decodeJson(char *jsonStr, char *attr, char *data) {
    unsigned int i = 0, j = 0;
    /* 匹配属性的位置，从1开始 */
    unsigned int pos = 0; 
    
	/*  */
	if(strlen(jsonStr) < strlen(attr)) return 0;
	
    /* 查找attr的位置 */
    for(i = 0; i < strlen(jsonStr) - strlen(attr) + 1; i++) {
		for(j = 0; j < strlen(attr); j++) {
			if(*(attr + j) == *(jsonStr + i + j)) continue;
			else break;
		}	
		if(j == strlen(attr)) {
			pos = i + 1;
			break;
		}
    } 

    if(pos == 0) return 0;

    /* 制作返回的字符串 */
    pos += strlen(attr);
    i = 0;
    while(1) {
		*(data + i) = *(jsonStr + pos + i); /* pos之所以不减1因为刚好跳过了等于号 */
		i++;
		if((*(jsonStr + pos + i) == '&') || ((*(jsonStr + pos + i) == '\0'))) break;
    }
    *(data + i) = '\0';

    return 1;
}

