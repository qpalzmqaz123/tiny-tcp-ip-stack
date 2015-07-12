#ifndef __ETH_H
#define __ETH_H

#include "datatype.h"

typedef struct ETHHeadStruct {
	UINT8 dstAdd[6]; /* 目标地址 */
	UINT8 srcAdd[6]; /* 源地址 */
	UINT16 type;     /* 上层协议类型 */
} ETHHeadStruct;

mIPErr ETH_analyzeHead(ETHHeadStruct *ptr, UINT32 *offset);
void  ETH_makeHead(ETHHeadStruct *ptr, UINT32 *offset);
void  ETH_makeHeadDefault(UINT32 *offset);

#endif
