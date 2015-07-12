#ifndef __MIPCONFIG_H
#define __MIPCONFIG_H

#define MYTCPIP_USE_FTP    1


/* 缓存区大小 */
#define myICPIP_bufSize    1550 /* 测得1550工作正常 */
#define myTCPIP_datBufSize 1550

/* ARP缓存条目最大数量 */
#define ARP_CACHE_MAXNUM     5

/* ARP缓存更新时间(单位为秒) */
#define ARP_CACHE_UPDATETIME 300

/* 等待ARP查询回应的时间(单位为毫秒),超过此时间认为局域网内没有相应主机 */
#define ARP_TIMEWAIT         500

/* TCP窗口的最大值 */
#define TCP_WINDOW_MAXSIZE   1460

/* TCP最大报文长度 */
#define TCP_OPTION_MSS       1460

/* 最大能建立的TCP连接数量 */
#define TCP_COON_MAXNUM      2 /* 经测试单线程时HTTP最好为1，FTP为2 */

/* TCP等待响应的超时时间(单位为毫秒) */
#define TCP_RETRY_TIME       500

/* TCP超时重试最大次数 */
#define TCP_RETRY_MAXNUM     5

#endif
