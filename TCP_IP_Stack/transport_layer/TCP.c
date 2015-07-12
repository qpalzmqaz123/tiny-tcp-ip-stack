#include "TCP.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量
 */
extern myTCPIPStruct mIP;


void tcpCoonOutput(void);

/**
 * @brief  解析TCP头
 * @param  ptr: TCP首部结构体指针，用来存放解析后数据
 * @param  offset: offset: buffer中偏移量指针
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
static mIPErr TCP_analyzeHead(TCPHeadStruct *ptr, UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	UINT8 i, tmp;
	
	ptr->srcPort = MKUINT16BIG(buf);
	buf += 2;
	ptr->dstPort = MKUINT16BIG(buf);
	buf += 2;
	ptr->squNum = MKUINT32BIG(buf);
	buf += 4;
	ptr->ackNum = MKUINT32BIG(buf);
	buf += 4;
	ptr->headLen = *buf++ >> 4;
	ptr->flag = *buf++ & 0x3F;
	ptr->window = MKUINT16BIG(buf);
	buf += 2;
	ptr->checkSum = MKUINT16BIG(buf);
	buf += 2;
	ptr->urgPtr = MKUINT16BIG(buf);
	buf += 2;
	bufCopy(ptr->option, buf, ptr->headLen * 4 - 20);
	buf += ptr->headLen * 4 - 20;
	
	ptr->MSS = 536; /* 默认536字节 */
	
	*offset = buf - mIP.buf;
	if(ptr->headLen * 4 - 20 == 0) return mIP_OK;
	
	/* 以下解析option(如果有的话) */
	for(i = 0; i < ptr->headLen * 4 - 20;) {
		if(ptr->option[i] == 1) { /* NOP */
			tmp = 1;
		}
		else {
			tmp = ptr->option[i + 1];
			
			if(ptr->option[i] == 2) { /* MSS */
				if(tmp == 4) {
					ptr->MSS = MKUINT16BIG(ptr->option + i + 2); /* 读取MSS */
				}
			}
			if(ptr->option[i] == 3) { /* 窗口扩大 */
				if(tmp == 3) {
					ptr->window <<= ptr->option[i + 2];
				}
			}
		}
		i += tmp;
	}
	
	return mIP_OK;
}


/**
 * @brief  通过TCP伪首部结构体制作TCP伪首部到buf中
 * @param  psPtr: TCP伪首部结构体
 * @param  buf: 存放解析后伪首部的buffer
 * @retval none
 */
static void TCP_makePsHead(TCPPseudoHeadStruct *psPtr, UINT8 *buf)
{
	bufCopy(buf, psPtr->srcIP, 4);
	buf += 4;
	bufCopy(buf, psPtr->dstIP, 4);
	buf += 4;
	*buf++ = psPtr->zero;                
	*buf++ = psPtr->protocol;
	UINT16TOPTRBIG(buf, psPtr->length);
}


/**
 * @brief  通过TCP首部结构体制作TCP首部到buffer中(实际上除了文件首部还有数据),伪首部不放入buffer，但要参与校验和的计算
 * @param  ptr: TCP首部指针
 * @param  psPtr: TCP伪首部指针
 * @param  offset: buffer偏移量指针
 * @param  data: 传送的数据指针
 * @param  dataLen: 传送的数据大小
 * @retval none
 */
static void TCP_makeHead(TCPHeadStruct *ptr, TCPPseudoHeadStruct *psPtr, UINT32 *offset, UINT8 *data, UINT32 dataLen)
{
	UINT8  *buf = mIP.buf + *offset;
	UINT8  psBuf[12];                /* 12字节的伪首部 */
	UINT16 sum;                      /* 校验和 */
	
	TCP_makePsHead(psPtr, psBuf);    /* 生成TCP首部,存入psBuf */
	
	/* 生成TCP首部 */
	UINT16TOPTRBIG(buf, ptr->srcPort);
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->dstPort);
	buf += 2;
	UINT32TOPTRBIG(buf, ptr->squNum);
	buf += 4;
	UINT32TOPTRBIG(buf, ptr->ackNum);
	buf += 4;
	*buf++ = ptr->headLen << 4;
	*buf++ = ptr->flag & 0x3F;
	UINT16TOPTRBIG(buf, ptr->window);
	buf += 2;
	UINT16TOPTRBIG(buf, 0x0000); /* 首先把校验和置0 */
	buf += 2;
	UINT16TOPTRBIG(buf, ptr->urgPtr);
	buf += 2;
	bufCopy(buf, ptr->option, ptr->headLen * 4 - 20); /* 复制option */
	buf += ptr->headLen * 4 - 20;
	bufCopy(buf, data, dataLen); /* 装载数据 */
	buf += dataLen;
	
    sum = calcuCheckSum2Buf(psBuf, 12, mIP.buf + *offset, ptr->headLen * 4 + dataLen); /* 生成校验和 */
	UINT16TOPTRBIG(mIP.buf + *offset + 16, sum);     /* 写入校验和 */
	
	*offset = buf - mIP.buf;
}

/**
 * @brief  使用TCP协议发送数据
 * @param  dstIP: 目标IP
 * @param  srcPort: 源端口，即本机发送TCP的端口
 * @param  dstPort: 目标端口
 * @param  seqNum: 序号
 * @param  ackNum: 确认号
 * @param  headLen: TCP首部大小，单位为4byte
 * @param  flag: 标志
 * @param  window: 窗口
 * @param  urgPtr: 紧急指针
 * @param  option: 长度可变的选项，其数据大小需要时间计算在headLen中
 * @param  data: 发送的数据的指针
 * @param  dataLen: 发送的数据的长度
 * @retval mIP_OK: 发送成功, mIP_ERROR: 发送失败
 */
mIPErr TCP_send(UINT8 *dstIP, UINT16 srcPort, UINT16 dstPort, UINT32 seqNum, UINT32 ackNum, UINT8 headLen, UINT8 flag, UINT16 window, UINT16 urgPtr, UINT8 *option, UINT8 *data, UINT32 dataLen)
{
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	ETHHeadStruct eth;
	UINT32 offset;
	UINT8 macTmp[6];
	
	/* 制作TCP首部 */
	tcp.srcPort  = srcPort;
	tcp.dstPort  = dstPort;
	tcp.squNum = seqNum;
	tcp.ackNum = ackNum;
	tcp.headLen = headLen;
	tcp.flag = flag;
	tcp.window = window;
	tcp.checkSum = 0x0000;
	tcp.urgPtr = urgPtr;
	bufCopy(tcp.option, option, headLen * 4 - 20);	
	
	/* 制作TCP伪首部 */
	bufCopy(tcpPs.srcIP, mIP.ip, 4);
	bufCopy(tcpPs.dstIP, dstIP, 4);
	tcpPs.zero = 0x00;
	tcpPs.protocol = IP_PROTOCOL_TCP;
	tcpPs.length = dataLen + headLen * 4;
	
	/* 发送数据前获取IP对应的MAC地址，如果获取不到返回ERROR */
	if(ARP_getMacByIp(mIP.ipTmp, macTmp) != mIP_OK) return mIP_NOACK;
	
	/* 偏移量归零，开始发送数据 */
	offset = 0;
	/* 写入ETH头 */
	bufCopy(eth.dstAdd, macTmp, 6);
	bufCopy(eth.srcAdd, mIP.mac, 6);
	eth.type = PROTOCOL_TYPE_IP;
	ETH_makeHead(&eth, &offset);
	/* 写入IP头 */
	IP_makeHeadDefault(tcp.headLen * 4 + dataLen, mIP.identification++, 0, 0, IP_PROTOCOL_TCP, &offset);
	/* 写入TCP头 */
	TCP_makeHead(&tcp, &tcpPs, &offset, data, dataLen);
	/* 发送数据 */
	myTCPIP_sendPacket(mIP.buf, offset);
	
	return mIP_OK;
}

/**
 * @brief  通过IP+端口号获得非关闭状态的TCP连接的在表中的位置
 * @param  srcIP: 源(客户端)IP
 * @param  srcPort: 源(客户端)端口
 * @param  dstPort: 目标(本机)端口
 * @retval 0: 表中不存在此IP+端口或连接已经关闭, !0: ip+端口在表(数组)中的位置，但是注意位置是从1开始的
 */
UINT32 TCP_getInfoPos(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* 遍历数组 */
		if( mIP.tcpTask->state[i] != TCP_COON_CLOSED &&        /* 连接没有关闭 */
			mIP.tcpTask->info[i].srcPort == srcPort &&         /* 源端口匹配 */
			mIP.tcpTask->info[i].dstPort == dstPort &&         /* 目标端口匹配 */
			bufMatch(mIP.tcpTask->info[i].srcIP, srcIP, 4)) {  /* IP地址匹配 */
			return (i + 1);
		}
	}
	
	return 0;
}

/**
 * @brief  通过IP+端口号添加一个连接信息到表中
 * @param  srcIP: 源(客户端)IP
 * @param  srcPort: 源(客户端)端口
 * @param  dstPort: 目标(本机)端口
 * @retval 0: 连接表中全是未关闭的连接，不能再添加了  !0: 添加成功,并且返回添加后的位置(从1开始)
 */
static UINT32 TCP_addInfo(UINT8 *srcIP, UINT16 srcPort, UINT16 dstPort)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* 遍历数组 */
		if(mIP.tcpTask->state[i] == TCP_COON_CLOSED) { /* 只有已经关闭的连接才能被覆盖掉 */
			bufCopy(mIP.tcpTask->info[i].srcIP, srcIP, 4);
			mIP.tcpTask->info[i].srcPort = srcPort;
			mIP.tcpTask->info[i].dstPort = dstPort;
			return (i + 1);
		}
	}
	
	return 0;
}

/**
 * @brief  TCP处理
 * @param  ip: 承载TCP的IP首部指针，主要用来计算TCP数据大小，校验等
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void TCP_process(IPHeadStruct *ip, UINT32 *offset)
{
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	UINT8 psBuf[12];             /* 12字节的伪首部 */
	UINT16 sum;
	UINT8 *buf = mIP.buf + *offset;
	UINT8 option[40];
	UINT32 pos;                  /* 存储连接在表中的位置 */
	UINT32 tmp;	
	UINT8  mssOption[4] = {0x02, 0x04, 0x04, 0x00}; /* MSS: 1024 */
	
	/* 设置伪首部 */
	bufCopy(tcpPs.srcIP, ip->srcAdd, 4);
	bufCopy(tcpPs.dstIP, ip->dstAdd, 4);
	tcpPs.zero = 0x00;
	tcpPs.protocol = ip->protocol;
	tcpPs.length = ip->totalLength - ip->headLength * 4;	
	TCP_makePsHead(&tcpPs, psBuf); /* 生成TCP伪首部,存入psBuf */
	
	sum = calcuCheckSum2Buf(psBuf, 12, buf, ip->totalLength - ip->headLength * 4); /* 计算校验和 */	
	if(sum != 0) return; /* 如果校验和不为0，丢包 */ 

	TCP_analyzeHead(&tcp, offset); /* 分析TCP首部 */
	
	if((tcp.flag & TCP_FLAG_SYN) && ((tcp.flag & TCP_FLAG_ACK) == 0)) {  /* SYN = 1 && ACK = 0表示客户端请求连接 */
		
		/*------------------------------------*/
		if(tcp.dstPort == TCP_PORT_FTP) {  /* 如果是FTP有点恶心的，21端口如果要下载会另外建立一个连接，调用回调函数 */
			coonSynCallBack(); 
		}
		/*------------------------------------*/
				
		if((pos = TCP_getInfoPos(ip->srcAdd, tcp.srcPort, tcp.dstPort)) != 0) { /* 这种情况是不应该出现的，如果出现了估计是网络问题导致之前的连接没关掉，那么重置标志位为SYNRCV即可 */
			mIP.tcpTask->state[pos - 1] = TCP_COON_SYNRCV; /* 同步收到 */
		}
		else { /* 如果没有错误，那么对于请求握手的连接应该添加到TCP连接信息表中 */
			pos = TCP_addInfo(ip->srcAdd, tcp.srcPort, tcp.dstPort); /* 添加连接到表中 */
			if(pos == 0) { /* 如果没能添加成功，那么就不响应 */
				return;
			}
			if(pos == 2 && tcp.dstPort == TCP_PORT_HTTP) { /* HTTP只能建立一个TCP连接 */
				mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
				return;
			}			
			mIP.tcpTask->info[pos - 1].serverWnd = TCP_WINDOW_MAXSIZE; /* 设置服务器可用窗口为本机最大值 */
			mIP.tcpTask->info[pos - 1].clientWnd = tcp.window; /* 设置客户端可用窗口为最大值 */
			mIP.tcpTask->info[pos - 1].clientMSS = tcp.MSS;    /* 设置客户端MSS */
			
			mIP.tcpTask->state[pos - 1] = TCP_COON_SYNRCV;     /* 如果添加成功，标记状态为为SYNRCV */
		}
		TCP_send(mIP.ipTmp, tcp.dstPort, tcp.srcPort, 
			0, tcp.squNum + 1,
			(20 + 4) / 4, TCP_FLAG_SYN | TCP_FLAG_ACK, TCP_WINDOW_MAXSIZE,
			0, mssOption, option, 0); /* 最后对握手请求作出响应 */
			
			mIP.tcpTask->info[pos - 1].squNumRcv = tcp.squNum;
			mIP.tcpTask->info[pos - 1].ackNumRcv = tcp.ackNum;
			mIP.tcpTask->info[pos - 1].squNumSnd = 0;
			mIP.tcpTask->info[pos - 1].ackNumSnd = tcp.squNum + 1; /* 不发送数据，但是要消耗一个序号 */
		return; /* 直接返回，别再往后面跑啦 */
	}
	
	pos = TCP_getInfoPos(ip->srcAdd, tcp.srcPort, tcp.dstPort);
	if(pos == 0) return; /* 实际上网络正常时pos是不可能等于0的，但是如果出问题就不应答，等对方重新握手 */
	
	/* 程序能跑到该注释后面说明连接信息表中确实有对应的连接信息 */
	
	mIP.tcpTask->info[pos - 1].squNumRcv = tcp.squNum;
	mIP.tcpTask->info[pos - 1].ackNumRcv = tcp.ackNum;
	mIP.tcpTask->info[pos - 1].squNumSnd = tcp.ackNum;
	mIP.tcpTask->info[pos - 1].ackNumSnd = tcp.squNum + ip->totalLength - ip->headLength * 4 - tcp.headLen * 4;
	
	if(mIP.tcpTask->state[pos - 1] == TCP_COON_SYNRCV && tcp.flag & TCP_FLAG_ACK) { /* 如果连接信息表中的标志位为SYNRCV并且TCP标志为ACK的话，说明这是客户端的第三次握手响应，说明连接完全建立，需修改标志为ESTAB */
		mIP.tcpTask->state[pos - 1] = TCP_COON_ESTAB;


#if MYTCPIP_USE_FTP == 1 		
		if(tcp.dstPort == TCP_PORT_FTP) { /* FTP需要主动应答 */
			mIP.tcpTask->info[pos - 1].dataLen = 0; /* 去bug */
			
			(*mIP.tcpCb)(&mIP.tcpTask->info[pos - 1]); /* 调用回调函数 */
		}	
#endif		
		
		if(tcp.dstPort != TCP_PORT_FTP) {  /* 每次建立完连接都会调用回调函数 */
		
			coonEstabCallBack(&mIP.tcpTask->info[pos - 1]);
		}
		
		return; /* 建立好连接就坐等客户端发请求了,别再往下跑了 */
	}
	
	if(tcp.flag & TCP_FLAG_RST) { /* 如果发来RST标志，那么必须关闭连接，必须.... */		
		mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
		return; /* 把该链接标记为关闭然后走人 */
	}

	if(tcp.flag & TCP_FLAG_ACK) { /* TCP规定，连接建立好以后所有传送的报文段必须把ACK置1，所以有必要检测一下 */
		if(tcp.flag & TCP_FLAG_FIN) { /* 如果对方想要关闭连接 */
			TCP_coonCloseAck(&mIP.tcpTask->info[pos - 1]);
		}
		else if(mIP.tcpTask->state[pos - 1] == TCP_COON_ESTAB) { /* 检测连接是否已经完整建立 */
			tmp = ip->totalLength - ip->headLength * 4; /* 首先计算IP携带数据的大小 */
			tmp -= tcp.headLen * 4;                     /* 然后计算TCP携带数据的大小 */
			
			mIP.tcpTask->info[pos - 1].data = buf + tcp.headLen * 4; /* 存储数据指针 */
			mIP.tcpTask->info[pos - 1].dataLen = tmp;                /* 存储数据大小 */
			if(tmp) (*mIP.tcpCb)(&mIP.tcpTask->info[pos - 1]);       /* 调用回调函数 */			
		}
	}
}

/**
 * @brief  等待or检查客户端的响应
 * @param  info: 回调函数参数
 * @param  mode: 0:仅检查是否有响应 1:等待响应才退出
 * @retval mIP_OK: 成功， mIP_NOACK: 超时
 */
mIPErr TCP_waitAck(TCPInfoStruct *info, UINT8 mode)
{
	ETHHeadStruct eth;
	TCPHeadStruct tcp;
	TCPPseudoHeadStruct tcpPs;
	UINT8 psBuf[12];             /* 12字节的伪首部 */
	UINT32 len, offset = 0;
	UINT32 time;  	
	IPHeadStruct ip;
	UINT16 sum;
	UINT8 *buf;
	
	time = myTCPIP_getTime();
	do {
		len = myTCPIP_getPacket(mIP.buf, myICPIP_bufSize);
		if(len == 0) continue;
		
		offset = 0; /* 设置buf的偏移量为0 */
		ETH_analyzeHead(&eth, &offset);
		
		if(eth.type == PROTOCOL_TYPE_ARP) {   /* ARP */
			ARP_process(&offset);
			return mIP_NOACK;
		}
		else if(eth.type == PROTOCOL_TYPE_IP) { /* IP */
			sum = calcuCheckSum(mIP.buf + offset, 20); /* IP只检验头部 */
			if(sum != 0) continue; /* 校验不通过,丢弃此包 */
			
			IP_analyzeHead(&ip, &offset); /* 解析IP固定的20byte头 */
			if(bufMatch(ip.dstAdd, mIP.ip, 4) == 0) continue; /* 如果目标地址不是本机IP，丢弃 */
			
			if(ip.protocol == IP_PROTOCOL_TCP) { /* 总算到了TCP */
				/* 设置伪首部 */
				bufCopy(tcpPs.srcIP, ip.srcAdd, 4);
				bufCopy(tcpPs.dstIP, ip.dstAdd, 4);
				tcpPs.zero = 0x00;
				tcpPs.protocol = ip.protocol;
				tcpPs.length = ip.totalLength - ip.headLength * 4;	
				TCP_makePsHead(&tcpPs, psBuf); /* 生成TCP伪首部,存入psBuf */
				
				buf = mIP.buf + offset;
				sum = calcuCheckSum2Buf(psBuf, 12, buf, ip.totalLength - ip.headLength * 4); /* 计算校验和 */	
				if(sum != 0) continue; /* 如果校验和不为0，丢包 */ 
				
				TCP_analyzeHead(&tcp, &offset); /* 分析TCP首部 */
				
				if(!(tcp.flag & TCP_FLAG_ACK)) continue;
				
				if(tcp.dstPort == info->dstPort && 
				   tcp.srcPort == info->srcPort &&
				   bufMatch(ip.srcAdd, info->srcIP, 4)) {
					
					info->squNumRcv = tcp.squNum;
					info->ackNumRcv = tcp.ackNum;
					info->data      = buf + tcp.headLen * 4;
					info->dataLen   = ip.totalLength - ip.headLength * 4 - tcp.headLen * 4;
					info->clientWnd = tcp.window;
					
					if(tcp.flag & TCP_FLAG_RST) return mIP_RST;      /* RST */
					else if(tcp.flag & TCP_FLAG_FIN) return mIP_FIN; /* FIN */
					
					return mIP_OK;
				}
			}
		} 
	} while( mode && ((myTCPIP_getTime() - time) <= TCP_RETRY_TIME) && (myTCPIP_getTime() >= time) ); /* 如果超时时间没到或者没有溢出 */
	
	return mIP_NOACK;
}

///**
// * @brief  多次回复开始
// * @param  data: 需发送的数据的指针 
// * @param  dataLen: 需发送的数据的长度
// * @param  info: 回调函数参数
// * @retval mIP_OK: 发送成功， mIP_ERROR:传入参数不符合规范
// */
//mIPErr TCP_replyMultiStart(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
//{
//	UINT8 optionMSS[4] = {0x02, 0x04};
//	
//	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
//	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

//	info->squNumSnd = info->ackNumRcv;                 /* 发送的squ应该等于接收的ack */
//	info->ackNumSnd = info->squNumRcv + info->dataLen; /* 期望的ack应该等于主机发送的序号+数据长度 */

//	TCP_send(info->srcIP,     /* 目标IP为源IP */
//			 info->dstPort,   /* 源端口为客户端发送的目标端口 */
//			 info->srcPort,   /* 目标端口为客户端的端口 */
//			 info->squNumSnd, /* 序号，前面已经处理好了 */
//			 info->ackNumSnd, /* 确认号，前面已经处理好了 */
//			 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
//			 TCP_FLAG_ACK,    /* 响应 */
//			 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
//			 0,               /* 紧急指针 */
//			 optionMSS,       /* MSS */
//			 data,            /* 数据指针 */
//			 dataLen          /* 数据长度 */
//			 ); 
//	info->squNumSnd = info->ackNumRcv + dataLen;

//	return mIP_OK;
//}

///**
// * @brief  回复但是不等待客户端响应
// * @param  data: 需发送的数据的指针 
// * @param  dataLen: 需发送的数据的长度
// * @param  info: 回调函数参数
// * @retval mIP_OK: 发送成功， mIP_ERROR:传入参数不符合规范
// */
//mIPErr TCP_replyMulti(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
//{	
//	UINT8 optionMSS[4] = {0x02, 0x04};
//	
//	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
//	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

//	TCP_send(info->srcIP,     /* 目标IP为源IP */
//			 info->dstPort,   /* 源端口为客户端发送的目标端口 */
//			 info->srcPort,   /* 目标端口为客户端的端口 */
//			 info->squNumSnd, /* 序号，前面已经处理好了 */
//			 info->ackNumSnd, /* 确认号，前面已经处理好了 */
//			 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
//			 TCP_FLAG_ACK,    /* 响应 */
//			 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
//			 0,               /* 紧急指针 */
//			 optionMSS,       /* MSS */
//			 data,            /* 数据指针 */
//			 dataLen          /* 数据长度 */
//			 );        
//    info->squNumSnd = info->squNumSnd + dataLen;                 /* 发送的squ应该等于接收的ack */

//	
//	return mIP_OK;
//}

/**
 * @brief  回复并且等待客户端的应答,如果超时自动重发，但是重发次数不能超过TCP_RETRY_MAXNUM
 * @param  data: 需发送的数据的指针 
 * @param  dataLen: 需发送的数据的长度
 * @param  info: 回调函数参数
 * @retval mIP_OK: 发送成功， mIP_ERROR:传入参数不符合规范,  mIP_NOACK: 客户端一直不应答并超过最大重试次数
 */
mIPErr TCP_replyAndWaitAck(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
{	
	mIPErr res;
	UINT8 optionMSS[4] = {0x02, 0x04};
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* 最大重试次数 */
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 

	info->squNumSnd = info->ackNumRcv;                 /* 发送的squ应该等于接收的ack */
	info->ackNumSnd = info->squNumRcv + info->dataLen; /* 期望的ack应该等于主机发送的序号+数据长度 */

	do {
		TCP_send(info->srcIP,     /* 目标IP为源IP */
				 info->dstPort,   /* 源端口为客户端发送的目标端口 */
				 info->srcPort,   /* 目标端口为客户端的端口 */
				 info->squNumSnd, /* 序号，前面已经处理好了 */
				 info->ackNumSnd, /* 确认号，前面已经处理好了 */
				 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
				 TCP_FLAG_ACK,    /* 响应 */
				 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
				 0,               /* 紧急指针 */
				 optionMSS,       /* MSS */
				 data,            /* 数据指针 */
				 dataLen          /* 数据长度 */
				 ); 
		res = TCP_waitAck(info, 1);
		if(res == mIP_FIN || res == mIP_RST) break;
	} while((res == mIP_NOACK) && (--retry)); 
	
//	return (retry) ? (mIP_OK) : (mIP_NOACK);
	return res;
}

/**
 * @brief  回复但是不会等待客户端应答
 * @param  data: 需发送的数据的指针 
 * @param  dataLen: 需发送的数据的长度
 * @param  info: 回调函数参数
 * @retval mIP_OK: 发送成功， mIP_ERROR:传入参数不符合规范,  mIP_NOACK: 客户端一直不应答并超过最大重试次数
 */
mIPErr TCP_reply(UINT8 *data, UINT32 dataLen, TCPInfoStruct *info)
{	
	UINT8 optionMSS[4] = {0x02, 0x04};
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS);
	if(dataLen > TCP_OPTION_MSS) return mIP_ERROR; 
	
	info->squNumSnd = info->ackNumRcv;                 /* 发送的squ应该等于接收的ack */
	info->ackNumSnd = info->squNumRcv + info->dataLen; /* 期望的ack应该等于主机发送的序号+数据长度 */

	TCP_send(info->srcIP,     /* 目标IP为源IP */
			 info->dstPort,   /* 源端口为客户端发送的目标端口 */
			 info->srcPort,   /* 目标端口为客户端的端口 */
			 info->squNumSnd, /* 序号，前面已经处理好了 */
			 info->ackNumSnd, /* 确认号，前面已经处理好了 */
			 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
			 TCP_FLAG_ACK,    /* 响应 */
			 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
			 0,               /* 紧急指针 */
			 optionMSS,       /* MSS */
			 data,            /* 数据指针 */
			 dataLen          /* 数据长度 */
			 );        
	
	return mIP_OK;
}

/**
 * @brief  主动关闭TCP连接
 * @param  info: 回调函数参数
 * @retval mIP_OK: 成功关闭 mIP_NOACK: 客户端没响应，但本机还是会关闭掉连接，释放资源 
 */
mIPErr TCP_coonClose(TCPInfoStruct *info)
{
	UINT8 optionMSS[4] = {0x02, 0x04};
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* 最大重试次数 */
	UINT32 pos;
	
	UINT16TOPTRBIG(optionMSS + 2, TCP_OPTION_MSS); 
	
	info->squNumSnd = info->ackNumRcv; /* 发送的squ应该等于接收的ack */
	info->ackNumSnd = info->squNumRcv; /* 关闭之前对方肯定回应了ACK(不带数据)，所以期望的ack应该等于主机发送的序号+0 */
	
	do {
		TCP_send(info->srcIP,     /* 目标IP为源IP */
				 info->dstPort,   /* 源端口为客户端发送的目标端口 */
				 info->srcPort,   /* 目标端口为客户端的端口 */
				 info->squNumSnd, /* 序号，前面已经处理好了 */
				 info->ackNumSnd, /* 确认号，前面已经处理好了 */
				 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
				 TCP_FLAG_ACK | TCP_FLAG_FIN, /* 响应&结束 */
				 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
				 0,               /* 紧急指针 */
				 optionMSS,       /* MSS */
				 optionMSS,       /* 数据指针 */
				 0                /* 数据长度 */
				 ); 
	} while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry));
	if(retry == 0) goto CLOSE;
	
/* 以上为：FINWAIT-1 我方FIN + ACK */
/* 以下为：FINWAIT-2 对方ACK */

	retry = TCP_RETRY_MAXNUM + 1;
	while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry));
	if(retry == 0) goto CLOSE;

/* 以上为：FINWAIT-3 对方FIN + ACK */	
/* 以下为：TIME-WAIT */

	info->squNumSnd = info->ackNumRcv;     /* 发送的squ应该等于接收的ack */
	info->ackNumSnd = info->squNumRcv + 1; /* 关闭之前对方肯定回应了ACK(不带数据)，所以期望的ack应该等于主机发送的序号+0 */	
	TCP_send(info->srcIP,     /* 目标IP为源IP */
			 info->dstPort,   /* 源端口为客户端发送的目标端口 */
			 info->srcPort,   /* 目标端口为客户端的端口 */
			 info->squNumSnd, /* 序号，前面已经处理好了 */
			 info->ackNumSnd, /* 确认号，前面已经处理好了 */
			 (20 + 4) / 4,    /* TCP首部长度：固有的20字节+MSS */
			 TCP_FLAG_ACK | TCP_FLAG_FIN, /* 响应&推送 */
			 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
			 0,               /* 紧急指针 */
			 optionMSS,       /* MSS */
			 optionMSS,       /* 数据指针 */
			 0                /* 数据长度 */
			 ); 

	/* 连接关闭后释放资源 */
CLOSE:	
	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_NOACK;  /* 实际上这是不可能发生的 */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return (retry) ? (mIP_OK) : (mIP_NOACK);
}

/**
 * @brief  被动关闭TCP连接，响应对方的FIN+ACK
 * @param  info: 回调函数参数
 * @retval mIP_OK: 成功关闭 mIP_NOACK: 客户端没响应，但本机还是会关闭掉连接，释放资源 
 */
mIPErr TCP_coonCloseAck(TCPInfoStruct *info)
{
	UINT8 option[1];
	UINT32 retry = TCP_RETRY_MAXNUM + 1; /* 最大重试次数 */
	UINT32 pos;
	UINT32 squ = info->ackNumRcv;
	
	do {
		TCP_send(info->srcIP,     /* 目标IP为源IP */
				 info->dstPort,   /* 源端口为客户端发送的目标端口 */
				 info->srcPort,   /* 目标端口为客户端的端口 */
				 squ,               /* 序号 */
				 info->squNumRcv + 1, /* 确认号 */
				 20 / 4,    /* TCP首部长度：固有的20字节+MSS */
				 TCP_FLAG_ACK,    /* 响应 */
				 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
				 0,               /* 紧急指针 */
				 option,          /* MSS */
				 option,          /* 数据指针 */
				 0                /* 数据长度 */
				 ); 

/* 以上为： CLOSE-WAIT */	
/* 以下为： LAST_ACK */
	
		TCP_send(info->srcIP,     /* 目标IP为源IP */
				 info->dstPort,   /* 源端口为客户端发送的目标端口 */
				 info->srcPort,   /* 目标端口为客户端的端口 */
				 squ,               /* 序号 */
				 info->squNumRcv + 1, /* 确认号 */
				 20 / 4,    /* TCP首部长度：固有的20字节+MSS */
				 TCP_FLAG_ACK | TCP_FLAG_FIN, /* 响应&结束 */
				 TCP_WINDOW_MAXSIZE, /* 这里以后需要修改 */
				 0,               /* 紧急指针 */
				 option,       /* MSS */
				 option,       /* 数据指针 */
				 0                /* 数据长度 */
				 ); 
	} while((TCP_waitAck(info, 1) == mIP_NOACK) && (--retry) && (info->ackNumRcv == squ + 1));
	
	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_NOACK;  /* 实际上这是不可能发生的 */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return (retry) ? (mIP_OK) : (mIP_NOACK);
}

/**
 * @brief  释放连接
 * @param  info: 回调函数参数
 * @retval mIP_OK: 成功释放 mIP_ERROR: 出错 
 */
mIPErr TCP_coonRelease(TCPInfoStruct *info)
{
	UINT32 pos;

	pos = TCP_getInfoPos(info->srcIP, info->srcPort, info->dstPort);
	if(pos == 0) return mIP_ERROR;  /* 实际上这是不可能发生的 */
	
	mIP.tcpTask->state[pos - 1] = TCP_COON_CLOSED;
	return mIP_OK;
}

/**
 * @brief  创建TCP任务(只能调用一次)
 * @param  ptr: TCP任务指针
 * @param  cbPtr: 回调函数指针，当建立好连接以后，如果客户端发送数据系统会自动调用回调函数
 * @retval none
 */
void TCP_createTask(TCPTaskStruct *ptr, void (*cbPtr)(TCPInfoStruct *))
{
	UINT32 i;

	mIP.tcpTask = ptr;
	mIP.tcpCb = cbPtr;
	mIP.enFlag |= ENFLAG_TCP; /* 使能TCP */
	mIP.enFlag |= ENFLAG_UDP;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) { /* 初始化所有连接的状态为关闭 */
		mIP.tcpTask->state[i] = TCP_COON_CLOSED; 
	}
}

/**
 * @brief  重置所有的TCP连接状态
 * @param  none
 * @retval none
 */
void TCP_coonReset(void)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		mIP.tcpTask->state[i] = TCP_COON_CLOSED;
	}
}

void tcpCoonOutput(void)
{
	UINT32 i;
	
	for(i = 0; i < TCP_COON_MAXNUM; i++) {
		printf("%d[%d] - ", i, mIP.tcpTask->state[i]);
	}
	printf("\r\n");
}



