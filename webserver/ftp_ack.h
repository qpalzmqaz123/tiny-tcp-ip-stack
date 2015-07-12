#ifndef __FTP_ACK_H
#define __FTP_ACK_H

#include "datatype.h"

/* SYST */
const char FTP_SYST[] = "UNIX Type: L8\r\n";

/* login */
const char FTP_ACK_220[] = "(myTCPIP_FTP ver1.0)\r\n";              /* 连接成功 */
const char FTP_ACK_331[] = "Please specify the password.\r\n";      /* 提示输入密码 */
const char FTP_ACK_230[] = "Login successful.\r\n";                 /* 登陆成功 */
const char FTP_ACK_530[] = "Login incorrect.\r\n";                  /* 用户名或密码登陆错误 */
const char FTP_ACK_503[] = "Login with USER first.\r\n";            /*  */
const char FTP_ACK_200[] = "switching to Binary mode.\r\n";         /*  */
const char FTP_ACK_200_U[] = "Always in UTF8 mode.\r\n";            /*  */
const char FTP_ACK_227[] = "Entering Passive Mode ";                /*  */
const char FTP_ACK_150[] = "Here comes the directroy listing.\r\n"; /* LIST */
const char FTP_ACK_150_D[] = "OK.\r\n";                             /* download */
const char FTP_ACK_226[] = "Directory send OK.\r\n";                /*  */
const char FTP_ACK_226_D[] = "Trasnfer complete.\r\n";               /*  */
const char FTP_ACK_250[] = "Directory successfully changed.\r\n";   /* 移动到某目录 */
                                                                 
#define FTP_ACK_211_NUM  9
const char FTP_ACK_211_START[] = "211-Features:\r\n";
const char FTP_ACK_211_END[]   = "211 End\r\n";
const char FTP_ACK_211[FTP_ACK_211_NUM][20] = {
" FEAT\r\n",
" EPRT\r\n",
" EPSV\r\n",
" MDTM\r\n",
" PASV\r\n",

" REST STREAM\r\n",
" SIZE\r\n",
" TVFS\r\n",
" UTF8\r\n"
};

#endif


