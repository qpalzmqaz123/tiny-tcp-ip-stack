#ifndef __FTP_H
#define __FTP_H

#include "datatype.h"
#include "tcp.h"

#define FTP_STU_IDLE       0 /* 空闲 */
#define FTP_STU_MATCHNAME  1  /* 匹配用户名 */
#define FTP_STU_MATCHPASS  2  /* 匹配密码 */
#define FTP_STU_LOGIN      3  /* 登陆成功 */

#define FTP_DATA_PORT 0x1414  /* 十六进制的14为十进制的20 */

typedef struct FTPTmpStruct {
	UINT32 ctlSquNumRcv;
	UINT32 ctlAckNumRcv;
	UINT32 datSquNumRcv;
	UINT32 datAckNumRcv;
} FTPTmpStruct;

void ftpProcess(TCPInfoStruct *info);
void ftpDataTsmit(TCPInfoStruct *info);

#endif

