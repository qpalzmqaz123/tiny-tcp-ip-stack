#include "ETH.h"
#include "myTCPIP.h"

/**
 * @brief 整个协议栈的全局变量 
 */
extern myTCPIPStruct mIP;

/**
 * @brief 网卡的全局变量，能临时存储接收的MAC地址 
 */
extern ETHHeadStruct ETHHead;


/**
 * @brief  分析以太网首部
 * @param  ptr: 以太网首部结构体指针
 * @param  offset: buffer中偏移量指针
 * @retval 错误标志，mIP_OK: 成功, mIP_ERROR: 失败
 */
mIPErr ETH_analyzeHead(ETHHeadStruct *ptr, UINT32 *offset)
{
	bufCopy(ptr->dstAdd, mIP.buf + *offset, 6);
	*offset += 6;
	bufCopy(ptr->srcAdd, mIP.buf + *offset, 6);
	*offset += 6;
	ptr->type = MKUINT16BIG(mIP.buf + *offset);
    *offset += 2;
	
	return mIP_OK;
}

/**
 * @brief  通过以太网首部结构体制作以太网首部到buffer中
 * @param  ptr: 以太网首部结构体指针
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void ETH_makeHead(ETHHeadStruct *ptr, UINT32 *offset)
{
	bufCopy(mIP.buf + *offset, ptr->dstAdd, 6);
    *offset += 6;
	bufCopy(mIP.buf + *offset, ptr->srcAdd, 6);
	*offset += 6;
	*(mIP.buf + *offset) = (UINT8)(ptr->type >> 8);
	*offset += 1;
	*(mIP.buf + *offset) = (UINT8)ptr->type;
	*offset += 1;
}

/**
 * @brief  制作默认以太网首部到buffer中,主要用于应答客户端时，源MAC和目标MAC和客户端发来的位置互换再发出
 * @param  offset: buffer偏移量指针
 * @retval none
 */
void ETH_makeHeadDefault(UINT32 *offset)
{
	UINT8 *buf = mIP.buf + *offset;
	
	bufCopy(buf, ETHHead.srcAdd, 6);
	buf += 6;
	bufCopy(buf, ETHHead.dstAdd, 6);
	buf += 6;
	UINT16TOPTRBIG(buf, ETHHead.type);
	buf += 2;
	*offset = buf - mIP.buf;
}

