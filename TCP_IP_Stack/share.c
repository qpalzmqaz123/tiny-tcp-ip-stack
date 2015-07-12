#include "share.h"

/**
 * @brief  复制src中的内容到dst中
 * @param  src: 源指针
 * @param  dst: 目标指针
 * @param  len: 复制的内容的长度
 * @retval none
 */
void bufCopy(UINT8 *dst, UINT8 *src, UINT32 len) 
{	
	if(len == 0) return;

	while(len--) {
		*dst++ = *src++;
	}
}

/**
 * @brief  检验两个buf的数据是否完全相同
 * @param  buf1: buf1的指针
 * @param  buf2: buf2的指针
 * @param  len: 检验的数据的长度
 * @retval 0: 不匹配， 1：匹配
 */
UINT8 bufMatch(UINT8 *buf1, UINT8 *buf2, UINT32 len)
{
	while(len--) {
		if(*buf1++ != *buf2++) return 0;
	}
	
	return 1;
}

/**
 * @brief  从str中抽取前len个字符然后查找它在buf中首次出现的位置
 * @param  buf: 目标的指针
 * @param  len1: 目标的最大长度 
 * @param  str: 查找源指针
 * @param  len2: str匹配有效长度
 * @retval 0: 找不到相应匹配， !0：匹配字符出现的位置，注意是从1开始的
 */
UINT32 strPos(UINT8 *buf, UINT32 len1, UINT8 *str, UINT32 len2)
{
	UINT32 i;

	for(i = 0; i < len1; i++) {
		if(bufMatch(buf + i, str, len2)) return i + 1;
	}
	
	return 0;
}

/**
 * @brief  计算校验和
 * @param  ptr: 需要计算校验和的buffer指针
 * @param  len: 需要计算数据部分的长度，单位为byte，如果len为奇数的话计算的时候会自动在尾部添加一个0x00
 * @retval 校验和
 */
UINT16 calcuCheckSum(UINT8 *ptr, UINT32 len)
{
	UINT32 sum = 0;

	while(len > 1) {
		sum += ((UINT16)*ptr << 8) | (UINT16)*(ptr + 1);
		ptr += 2;
		len -= 2;	
	}
	if(len) { /* 如果长度是奇数需要尾部补0 */
		sum += ((UINT16)*ptr << 8) | 0x00;
	}
	while(sum & 0xFFFF0000) {
		sum = (sum & 0x0000FFFF) + (sum >> 16);
	}
	
	return (UINT16)(~sum); /* 返回和的反码 */
}

/**
 * @brief  通过两个buffer计算校验和(主要用于UDP，TCP伪首部和实际首部&数据的校验)
 * @param  buf1: buffer1的指针
 * @param  len1: buffer1需要计算数据部分的长度，单位为byte
 * @param  buf2: buffer2的指针
 * @param  len2: buffer2需要计算数据部分的长度，单位为byte
 * @retval 校验和
 */
UINT16 calcuCheckSum2Buf(UINT8 *buf1, UINT32 len1, UINT8 *buf2, UINT32 len2)
{
	UINT32 sum = 0;	

	sum += (UINT16)~calcuCheckSum(buf1, len1);
	sum += (UINT16)~calcuCheckSum(buf2, len2);
	while(sum & 0xFFFF0000) {
		sum = (sum & 0x0000FFFF) + (sum >> 16);
	}
	
	return ~sum;
}

