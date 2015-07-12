#ifndef __SHARE_H
#define __SHARE_H

#include "datatype.h"

#define MKUINT16BIG(ptr) (UINT16)( ((UINT16)*(ptr) << 8) | (UINT16)*((ptr) + 1) )
#define MKUINT16LIT(ptr) (UINT16)( (UINT16)*(ptr) | ((UINT16)*((ptr) + 1) << 8) )

#define MKUINT32BIG(ptr) (UINT32)( ((UINT32)*(ptr) << 24) | ((UINT32)*((ptr) +  1) << 16) | ((UINT32)*((ptr) + 2) << 8) | (UINT32)*((ptr) + 3) )
#define MKUINT32LIT(ptr) (UINT32)(  (UINT32)*(ptr) | ((UINT32)*((ptr) + 1) << 8) | ((UINT32)*((ptr) + 2) << 16) | ((UINT32)*((ptr) + 3) << 24) )

#define UINT16TOPTRBIG(ptr, dat) { *(ptr) = (UINT8)((dat) >> 8); *((ptr) + 1) = (UINT8)(dat); } 
#define UINT16TOPTRLIT(ptr, dat) { *(ptr) = (UINT8)(dat); *((ptr) + 1) = (UINT8)((dat) >> 8); }

#define UINT32TOPTRBIG(ptr, dat) { *(ptr) = (UINT8)((dat) >> 24); *((ptr) + 1) = (UINT8)((dat) >> 16); *((ptr) + 2) = (UINT8)((dat) >> 8); *((ptr) + 3) = (UINT8)(dat); } 
#define UINT32TOPTRLIT(ptr, dat) { *(ptr) = (UINT8)(dat); *((ptr) + 1) = (UINT8)((dat) >> 8); *((ptr) + 2) = (UINT8)((dat) >> 16); *((ptr) + 3) = (UINT8)((dat) >> 24); }

void bufCopy(UINT8 *dst, UINT8 *src, UINT32 len);
UINT8 bufMatch(UINT8 *buf1, UINT8 *buf2, UINT32 len);
UINT16 calcuCheckSum(UINT8 *ptr, UINT32 len);
UINT16 calcuCheckSum2Buf(UINT8 *buf1, UINT32 len1, UINT8 *buf2, UINT32 len2);
UINT32 strPos(UINT8 *buf, UINT32 len1, UINT8 *str, UINT32 len2);

#endif
