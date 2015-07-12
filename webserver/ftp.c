#include "ftp.h"
#include "ftp_ack.h"
#include "myTCPIP.h"
#include <stdio.h>
#include <string.h>
#include "ff.h"

extern myTCPIPStruct mIP;

#define FTP_DATA_PORT 0x1414

extern FATFS   fs;
extern FIL     file;
extern DIR     dir;
extern FILINFO fileInfo;

static const char ftpUser[] = "root\r\n";           /* 用户名 */
static const char ftpPass[] = "stm32webserver\r\n"; /* 密码 */
static const char ftpPath[] = "/";					/* 目录 */
static char       ftpPathTmp[255] = "/";            /* 临时目录 */
static char       ftpPathTmp_BAK[255];              /* 备份 */ 
static char       ftpPathBuf[1024 * 2];             /* 缓存目录下的所有文件信息 */
static char       ftpDownBuf[255];                  /* 需要下载的文件名 */
static TCPInfoStruct infoStruct_BAK;

//#define FTP_DATA_PORT 0x1414  ！！声明在ftp.h中
static const char ftpPasv[] = "(169,254,228,100,20,20).\r\n";

static UINT8 resBuf[260]; 
static UINT8 tinyBuf[80];                          /* 回应的缓存区 */
static UINT8 pathBuf[255];

UINT8 ftpStatus = FTP_STU_IDLE;
UINT8 ftpStatus_BAK = FTP_STU_IDLE;
UINT8 ftpMatchUserStu = 0;
UINT8 ftpDataFlag = 0;
UINT8 ftpFirst    = 0;
#define FTP_DATA_FLAG_DIRLIST  1
#define FTP_DATA_FLAG_DOWNLOAD 2
#define FTP_DATA_FLAG_UPLOAD   3

static UINT8 uploadFin = 0; /* 去除上传后不能立即关闭连接的问题 */


void statusBackup(void);
void statusRestote(void);
mIPErr TCP_saveFile(TCPInfoStruct *info);

mIPErr ftpResponse(UINT16 res, UINT8 *data, UINT8 dataLen, TCPInfoStruct *info, UINT8 enEOF)
{
	mIPErr miperr;
	UINT32 len;

	if(res != 0)
		sprintf((char *)resBuf, "%d ", res);
	else
		resBuf[0] = '\0';
		
	len = strlen((char *)resBuf) + dataLen;	
	bufCopy(resBuf + strlen((char *)resBuf), data, dataLen);
	resBuf[len] = '\0';
	
	if(enEOF) strcat((char *)resBuf, "\r\n");

//	miperr = TCP_replyAndWaitAck(resBuf, strlen((char *)resBuf), info);
	miperr = TCP_reply(resBuf, strlen((char *)resBuf), info);
//	TCP_reply(resBuf, 0, info); /* ACK */

/////////////
	info->ackNumRcv += dataLen + 4;
/////////////	
	
	return miperr;
}


/**
 * @brief 读取相应目录下的所有文件到buf中
 */
void mkDirRsp(char *path, char *buf)
{
	char sizeBuf[10];

	f_opendir(&dir, ftpPathTmp);
	*buf = '\0';
	while(1) {
		f_readdir(&dir, &fileInfo);
		if(fileInfo.fname[0] == 0) break; /* 若没有文件了，退出 */
		
		if(fileInfo.fattrib & AM_DIR) strcat(buf, "d");   /* 目录为'd'，普通文件为'-' */
		else                          strcat(buf, "-");
			
		strcat(buf, "rwxrwxrwx    2 0        0            "); /* 通用 */
		sprintf(sizeBuf, "%d ", (int)fileInfo.fsize);
		strcat(buf, sizeBuf);                                 /* 文件大小 */
		
		if(fileInfo.lfname[0]) strcat(buf, fileInfo.lfname);  /* 长文件名 */
		else                   strcat(buf, fileInfo.fname);
		
		strcat(buf, "\r\n");
	}
}

mIPErr TCP_sendDataFast(UINT32 mss, UINT32 size, TCPInfoStruct *info);
/**
 * @brief 数据通道 
 */
void ftpDatTsmit(TCPInfoStruct *info)
{
	mIPErr res;
	UINT32 mss, size;
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	switch(ftpDataFlag) {
		case FTP_DATA_FLAG_DIRLIST:
			
			mkDirRsp(ftpPathTmp, ftpPathBuf);			
			ftpResponse(150, (UINT8 *)FTP_ACK_150, strlen(FTP_ACK_150), p, 0);
		
			TCP_replyAndWaitAck((UINT8 *)ftpPathBuf, strlen(ftpPathBuf), info);
			
	
			TCP_send(
				info->srcIP, info->dstPort, info->srcPort,
				info->ackNumRcv, info->squNumRcv,
				20/4,
				TCP_FLAG_ACK | TCP_FLAG_FIN,
				TCP_WINDOW_MAXSIZE,
				0,
				(UINT8 *)tinyBuf, (UINT8 *)tinyBuf, 0
			);
			TCP_waitAck(info, 1);

			ftpResponse(226, (UINT8 *)FTP_ACK_226, strlen(FTP_ACK_226), p, 0);
			
			/* 以下为了去除upload后不能立即关闭连接的问题 */
			if(uploadFin == 1) {
				uploadFin = 0;
				TCP_coonClose(p);
				statusRestote(); /* 恢复 */
			}
			break;
			
		case FTP_DATA_FLAG_DOWNLOAD:
			strcpy(ftpPathTmp_BAK, ftpPathTmp);

printf("DownLoad: %s\r\n", ftpDownBuf);
			ftpResponse(150, (UINT8 *)FTP_ACK_150_D, strlen(FTP_ACK_150_D), p, 0);
		
			mss = (TCP_OPTION_MSS > info->clientMSS) ? (info->clientMSS) : (TCP_OPTION_MSS); /* 计算一次能发出的最大数据量 */
			res = TCP_vndOpen((UINT8 *)ftpDownBuf, &size, METHOD_FATFS); /* 首先判断这请求的页面是否存在 */
			
			if(res != mIP_OK) return;
			
			res = TCP_sendDataFast(mss, size, info);	
			
			if(res == mIP_NOACK) { /* 如果对方不响应则主动关闭连接 */
				TCP_coonClose(info);
			}
			else if(res == mIP_FIN) { /* 如果对方主动关闭连接 */
				TCP_coonCloseAck(info);
			}
			else if(res == mIP_RST) { /* 如果对方发送RST则立马释放连接资源 */
				TCP_coonRelease(info);
			}
			
			TCP_vndClose(METHOD_FATFS); /* 关闭文件指针 */
		
			ftpResponse(226, (UINT8 *)FTP_ACK_226_D, strlen(FTP_ACK_226_D), p, 0);
TCP_waitAck(p, 1);
			TCP_coonClose(p);
			
			statusRestote(); /* 恢复 */
			
printf("RET*****Client: %d -- Server: %d\r\n", p->srcPort, p->dstPort);
tcpCoonOutput();
printf("Status: %d", ftpStatus);			
			
			break;
			
		case FTP_DATA_FLAG_UPLOAD:
			strcpy(ftpPathTmp_BAK, ftpPathTmp);
printf("upload\r\n");
			res = TCP_vndWriteReady((UINT8 *)ftpDownBuf, METHOD_FATFS);
			if(res != mIP_OK) return;
			
			ftpResponse(150, (UINT8 *)FTP_ACK_150_D, strlen(FTP_ACK_150_D), p, 0); /* 开始传输数据 */
			res = TCP_saveFile(info); /* 保存文件 */

			if(res == mIP_NOACK) { /* 如果对方不响应则主动关闭连接 */
				TCP_coonClose(info);
			}
			else if(res == mIP_FIN) { /* 如果对方主动关闭连接 */
				TCP_coonCloseAck(info);
			}
			else if(res == mIP_RST) { /* 如果对方发送RST则立马释放连接资源 */
				TCP_coonRelease(info);
			}
			TCP_vndClose(METHOD_FATFS); /* 关闭文件指针 */
printf("ok\r\n");
			
			ftpResponse(226, (UINT8 *)FTP_ACK_226_D, strlen(FTP_ACK_226_D), p, 0);
TCP_waitAck(p, 1);
			
			uploadFin = 1;
//			TCP_coonClose(p);
			
//			statusRestote(); /* 恢复 */
			
			break;
			
		default: 
			ftpResponse(220, (UINT8 *)FTP_ACK_220, strlen(FTP_ACK_220), info, 0);
			break;
	}
}


/**
 * @brief 请求建立第二个FTP时的回调函数 
 */
void coonSynCallBack(void)
{
	unsigned int i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		if(mIP.tcpTask->info[i].dstPort == 21 || 1) { /////////////////////////////////////////////////////////
			mIP.tcpTask->state[i] = TCP_COON_CLOSED;
			if(mIP.tcpTask->info[i].dstPort == 21) {
				ftpStatus   = FTP_STU_IDLE;
				ftpDataFlag = FTP_DATA_FLAG_DOWNLOAD;
			}
		}
	}
}


/**
 * @brief FTP处理 
 */
void ftpProcess(TCPInfoStruct *info)
{
	mIPErr res;
	UINT8 tmp;
	UINT32 i;
	char tmpBuf[255];
	
	if(ftpFirst == 0) {
		statusBackup(); /* 备份 */
		printf("Client: %d -- Server: %d\r\n", infoStruct_BAK.srcPort, infoStruct_BAK.dstPort);
	}
//	printf("Client: %d -- Server: %d\r\n", infoStruct_BAK.srcPort, infoStruct_BAK.dstPort);
	
	ftpFirst = 1;
	
	switch(ftpStatus) {
		/* 连接建立好之后回应220 */
		case FTP_STU_IDLE:
			ftpResponse(220, (UINT8 *)FTP_ACK_220, strlen(FTP_ACK_220), info, 0);
			ftpStatus = FTP_STU_MATCHNAME;
			goto QUIT;
			
		/* 匹配用户名 */
		case FTP_STU_MATCHNAME:       
			ftpMatchUserStu = 0;
			if(bufMatch(info->data, (UINT8 *)"USER", 4) == 0) {
				TCP_reply(resBuf, 0, info);
				ftpResponse(503, (UINT8 *)FTP_ACK_503, strlen(FTP_ACK_503), info, 0);
				goto QUIT;
			}
			if(bufMatch(info->data + 5, (UINT8 *)ftpUser, strlen(ftpUser))) {
				ftpMatchUserStu = 1; /* 用户名匹配成功 */
			}
			TCP_reply(resBuf, 0, info);
			ftpResponse(331, (UINT8 *)FTP_ACK_331, strlen(FTP_ACK_331), info, 0);
			ftpStatus = FTP_STU_MATCHPASS;
			goto QUIT;
			
		/* 匹配密码 */
		case FTP_STU_MATCHPASS:
			if(bufMatch(info->data, (UINT8 *)"PASS", 4)) {
				if(bufMatch(info->data + 5, (UINT8 *)ftpPass, 4)) { /* 登陆成功 */
			        ftpStatus = FTP_STU_LOGIN;
					TCP_reply(resBuf, 0, info);
					strcpy(ftpPathTmp, ftpPath);                    /* copy根目录到工作区 */      
					ftpResponse(230, (UINT8 *)FTP_ACK_230, strlen(FTP_ACK_230), info, 0);
					goto QUIT;
				}
			}
			TCP_reply(resBuf, 0, info);
			ftpResponse(530, (UINT8 *)FTP_ACK_530, strlen(FTP_ACK_530), info, 0);	
			goto QUIT;
			
		/* 登陆成功 */
		case FTP_STU_LOGIN:
			if(bufMatch(info->data, (UINT8 *)"SYST", 4)) {  
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(215, (UINT8 *)FTP_SYST, strlen(FTP_SYST), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"FEAT", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(0, (UINT8 *)FTP_ACK_211_START, strlen(FTP_ACK_211_START), info, 0);
				TCP_waitAck(info, 1);
				for(tmp = 0; tmp < FTP_ACK_211_NUM; tmp++) {
					res = ftpResponse(0, (UINT8 *)FTP_ACK_211[tmp], 7, info, 0);
					TCP_waitAck(info, 1);
				}
				res = ftpResponse(0, (UINT8 *)FTP_ACK_211_END, strlen(FTP_ACK_211_END), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"OPTS", 4)) {
				if(bufMatch(info->data + 5, (UINT8 *)"UTF8 ON", 7)) {
					ftpResponse(200, (UINT8 *)FTP_ACK_200_U, strlen(FTP_ACK_200_U), info, 0);
				}
			}
			else if(bufMatch(info->data, (UINT8 *)"TYPE", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(200, (UINT8 *)FTP_ACK_200, strlen(FTP_ACK_200), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"PASV", 4)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				bufCopy(tinyBuf, (UINT8 *)FTP_ACK_227, strlen(FTP_ACK_227) + 1);
				strcat((char *)tinyBuf, ftpPasv);
				res = ftpResponse(227, (UINT8 *)tinyBuf, strlen((char *)tinyBuf), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"LIST", 4)) {
				ftpDataFlag = FTP_DATA_FLAG_DIRLIST;
			}
			else if(bufMatch(info->data, (UINT8 *)"CDUP", 4)) {
				strcpy(ftpPathTmp, ftpPath);
				TCP_reply(resBuf, 0, info); /* ACK */
				ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
			}
			else if(bufMatch(info->data, (UINT8 *)"PWD", 3)) {
				TCP_reply(resBuf, 0, info); /* ACK */
				strcpy(tmpBuf, "\"");
				strcat(tmpBuf, ftpPathTmp);
				strcat(tmpBuf, "\"\r\n");
				ftpResponse(257, (UINT8 *)tmpBuf, strlen(tmpBuf), info, 0);
printf("PWD: %s\r\n", ftpPathTmp);			
			}
			else if(bufMatch(info->data, (UINT8 *)"RETR", 4)) {
printf("RETR: %s\r\n", info->data + 5);
				ftpDataFlag = FTP_DATA_FLAG_DOWNLOAD;
				strcpy(ftpDownBuf, ftpPathTmp);                /* 复制目录 */
				strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)info->data + 5);    /* 复制文件名 */
				for(i = 0; i < 2000; i++) { /* delete "\r\n" */
					if(*(ftpDownBuf + i) == '\r') {
						*(ftpDownBuf + i) = '\0';
						break;
					}
				}
			}
			else if(bufMatch(info->data, (UINT8 *)"CWD", 3)) {
				if(*(info->data + 4) == '/') { /* 如果是绝对目录 */
					strcpy(ftpPathTmp, (char *)info->data + 4);
				}
				else { /* 相对目录 */
					if(strlen(ftpPathTmp) != 1) /* 长度不为1说明不是root目录 */
						strcat(ftpPathTmp, "/");
					strcat(ftpPathTmp, (char *)info->data + 4);
				}
				for(i = 0; i < 2000; i++) { /* delete "\r\n" */
					if(*(ftpPathTmp + i) == '\r') {
						*(ftpPathTmp + i) = '\0';
						break;
					}
				}
				TCP_reply(resBuf, 0, info); /* ACK */
				res = ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
				
printf("CWD: %s\r\n", ftpPathTmp);
			}
			
			else if(bufMatch(info->data, (UINT8 *)"STOR", 4)) { /* 上传文件 */
				ftpDataFlag = FTP_DATA_FLAG_UPLOAD;
				
				strcpy(ftpDownBuf, ftpPathTmp);
				if(strlen(ftpPathTmp) != 1) /* 长度不为1说明不是root目录 */
					strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)(info->data + 5));
				
printf("STOR: %s\r\n", ftpDownBuf);
			}
			
			else if(bufMatch(info->data, (UINT8 *)"DELE", 4)) { /* 删除文件 */
				TCP_reply(resBuf, 0, info); /* ACK */
				
				strcpy(ftpDownBuf, ftpPathTmp);
				if(strlen(ftpPathTmp) != 1) /* 长度不为1说明不是root目录 */
					strcat(ftpDownBuf, "/");
				strcat(ftpDownBuf, (char *)(info->data + 5));
				
//				f_unlink(ftpDownBuf); /* 删除文件 */
				TCP_vndClose(METHOD_FATFS);
printf("delete file: %s: -%x\r\n", ftpDownBuf,  f_unlink(ftpDownBuf));				
				ftpResponse(250, (UINT8 *)FTP_ACK_250, strlen(FTP_ACK_250), info, 0);
			}
			
			goto QUIT;
			
		default: break;
	}
	
	QUIT: tmp = tmp;
}

/**
 * @brief 备份链接信息
 */
void statusBackup(void)
{
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	strcpy(ftpPathTmp_BAK, ftpPathTmp);
	ftpStatus_BAK = ftpStatus;
	bufCopy((UINT8 *)&infoStruct_BAK, (UINT8 *)p, sizeof(infoStruct_BAK));
}

/**
 * @brief 恢复链接信息
 */
void statusRestote(void)
{
	TCPInfoStruct *p;
	p = &((mIP.tcpTask->info)[0]);
	
	mIP.tcpTask->state[0] = TCP_COON_ESTAB; /* 打开链接 */
	strcpy(ftpPathTmp, ftpPathTmp_BAK);
//	ftpStatus = ftpStatus_BAK;
	ftpStatus = FTP_STU_LOGIN;
	bufCopy((UINT8 *)p, (UINT8 *)&infoStruct_BAK, sizeof(infoStruct_BAK));
}


/**
 * @brief 读取上传文件并存入SD卡
 */
mIPErr TCP_saveFile(TCPInfoStruct *info)
{
	mIPErr ack;
	UINT32 first = 1;
	UINT32 i, squNum, ackNum;
	UINT32 squNumSnd, ackNumSnd, writeEn;
	UINT8  mssOption[4] = {0x02, 0x04, 0x05, 0xb4}; /* MSS: 1460 */

	UINT8  buf[1500];
	
	while(1) {
		ack = TCP_waitAck(info, 0);       /* 检查客户端是否发出了数据 */
		if(ack == mIP_OK) {
			squNum = info->squNumRcv;     /* 记录squ */
			ackNum = info->ackNumRcv;     /* 记录ack */
			if(first == 1) {              /* 首次需要记录但是不判断 */
				first  = 0;               /* 清除首次标志 */
				writeEn = 1;              /* 使能写入 */
				squNumSnd = ackNum;       /* 己方序号为对方期望号 */
				ackNumSnd = squNum + info->dataLen; /* 己方期望号为对方序列号加上数据长度 */
//printf("squ: %X -- ack: %X\r\n", squNumSnd, ackNumSnd);
			}
			else {                        /* 如果不是首次需要保证文件连续性 */
				if(squNum != ackNumSnd) { /* 如果目前接收到的数据因为网络问题不是连续的 */
					writeEn = 0;          /* 清除写使能标志 */
				}
				else {                    /* 如果数据是连续的 */
					squNumSnd = ackNum;   /* 己方序号为对方期望号 */
					ackNumSnd = squNum + info->dataLen; /* 己方期望号为对方序列号加上数据长度 */
					writeEn = 1;          /* 使能写入 */
				}
			}
			
			if(writeEn) {                 /* 只有写标志为1才写入SD卡 */
				bufCopy(buf, info->data, info->dataLen);        /* 复制data */
				TCP_vndWrite(buf, info->dataLen, METHOD_FATFS); /* 写入到SD卡 */
			}			
			
			TCP_send(info->srcIP,         /* 目标IP为源IP */
				 info->dstPort,           /* 源端口为客户端发送的目标端口 */
				 info->srcPort,           /* 目标端口为客户端的端口 */
				 squNumSnd,               /* 序号 */
				 ackNumSnd,               /* 确认号 */
				 20 / 4,                  /* TCP首部长度：固有的20字节 */
				 TCP_FLAG_ACK,            /* 响应 */
				 TCP_WINDOW_MAXSIZE,      /* 这里以后需要修改 */
				 0,                       /* 紧急指针 */
				 mssOption,               /* MSS */
				 tinyBuf,                 /* 数据指针 */
				 0                        /* 数据长度 */
				 );
		}
		else if(ack == mIP_RST) {
			return mIP_RST;
		}
		else if(ack == mIP_FIN) {
			if(info->dataLen != 0) {
				bufCopy(buf, info->data, info->dataLen);        /* 复制data */
				TCP_vndWrite(buf, info->dataLen, METHOD_FATFS); /* 写入到SD卡 */
			}
			return mIP_FIN;
		}
	}
	return mIP_OK;
}

