#ifndef __DATATYPE_H
#define __DATATYPE_H

typedef char            CHAR;

typedef unsigned char  UINT8;
typedef char            INT8;

typedef unsigned short UINT16;
typedef          short  INT16;

typedef unsigned long  UINT32;
typedef          long   INT32;

typedef enum mIPErr{
	mIP_OK = 0, /* 成功 */
	mIP_ERROR,  /* 失败 */
	mIP_NOACK,  /* 无应答 */
	mIP_RST,    /* 复位 */
	mIP_FIN     /* 关闭 */
} mIPErr;

typedef enum VND_METHOD {
	METHOD_FATFS, /* fatfs文件系统 */
	METHOD_RAM    /* RAM */
} VND_METHOD;

#endif
