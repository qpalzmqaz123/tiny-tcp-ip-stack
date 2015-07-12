/******************** (C) COPYRIGHT 2011 迷你嵌入式开发工作室 ********************
 * 文件名  ：simple_server.c
 * 描述    ：web服务程序应用函数库
 *          
 * 实验平台：野火STM32开发板
 * 硬件连接： ------------------------------------
 *           |PB13         ：ENC28J60-INT (没用到)|
 *           |PA6-SPI1-MISO：ENC28J60-SO          |
 *           |PA7-SPI1-MOSI：ENC28J60-SI          |
 *           |PA5-SPI1-SCK ：ENC28J60-SCK         |
 *           |PA4-SPI1-NSS ：ENC28J60-CS          |
 *           |PE1          ：ENC28J60-RST (没用)  |
 *            ------------------------------------
 * 库版本  ：ST3.0.0
 *
 * 作者    ：fire  QQ: 313303034
 * 博客    ：firestm32.blog.chinaunix.net
**********************************************************************************/
#include "enc28j60.h"
#include "ip_arp_udp_tcp.h"
#include "net.h"
#include "web_server.h"
#include <string.h>


#define PSTR(s) s	

/* LED1 亮灭宏定义 */
//#define LED1_ON()  GPIO_SetBits(GPIOC, GPIO_Pin_3);
//#define LED1_OFF()  GPIO_ResetBits(GPIOC, GPIO_Pin_3);


/* mac地址和ip地址在局域网内必须唯一，否则将与其他主机冲突，导致连接不成功 */
static unsigned char mymac[6] = {0x54,0x55,0x58,0x10,0x00,0x24};
static unsigned char myip[4] = {169,254,228,96};

/* ip地址(或者是DNS的名字，如果有DNS服务器的话)，ip地址必须以"/"结尾 */
static char baseurl[]="http://169.254.228.96/";

/* tcp/www监听端口号，范围为:1-254 */
static unsigned int mywwwport =80; 

/* udp 监听端口号，即本地(开发板)端口号 */
static unsigned int myudpport =1200; 

/* 发送数据缓冲区 */
#define BUFFER_SIZE 1500
static unsigned char buf[BUFFER_SIZE+1];

/* 密码,不能大于9个字符(只有密码的前5位会被检测)，(字符限定为：a-z,0-9) */
static char password[]="123456"; 

/*
 * 函数名：verify_password
 * 描述  ：确认密码
 * 输入  ：str
 * 输出  ：无
 * 返回  ：
 */ 
unsigned char verify_password(char *str)
{
    // the first characters of the received string are
    // a simple password/cookie:
    if (strncmp(password,str,5)==0)
    {
        return(1);
    }
    return(0);
}

/*
 * 函数名：analyse_get_url
 * 描述  ：takes a string of the form password/commandNumber and analyse it
 * 输入  ：str
 * 输出  ：-1 invalid password, otherwise command number
 *         -2	no command given but password valid
 * 返回  ：
 */
unsigned char analyse_get_url(char *str)
{
    unsigned char i=0;
    if (verify_password(str)==0)
    {
        return(-1);
    }
    // find first "/"
    // passw not longer than 9 char:
    while(*str && i<10 && *str >',' && *str<'{')
    {
        if (*str=='/')
        {
            str++;
            break;
        }
        i++;
        str++;
    }
    if (*str < 0x3a && *str > 0x2f)
    {
        // is a ASCII number, return it
        return(*str-0x30);
    }
    return(-2);
}

/*
 * 函数名：print_webpage
 * 描述  ：将数据写到tcp的发送缓冲区(以实现一个网页)
 * 输入  ：-buf
 *         -on_off
 * 输出  ：无
 * 返回  ：-plen
 */
unsigned int print_webpage(unsigned char *buf,unsigned char on_off)
{
    unsigned int plen;
		/* 新建一个网页，就像新建一个文件一样 */
    plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nPragma: no-cache\r\n\r\n"));
	
		/* 在网页正中央填写信息 */
    plen=fill_tcp_data_p(buf,plen,PSTR("<center><p> LED状态指示: "));

    if (on_off)
    {
        plen=fill_tcp_data_p(buf,plen,PSTR("<font color=\"#00FF00\"> 亮</font>"));
    }
    else
    {
        plen=fill_tcp_data_p(buf,plen,PSTR("灭"));
    }

    plen=fill_tcp_data_p(buf,plen,PSTR(" <small><a href=\""));
    plen=fill_tcp_data(buf,plen,baseurl);		 // 要在浏览器中输入的网址
    plen=fill_tcp_data(buf,plen,password);	 // 紧跟在网址后面的密码
    plen=fill_tcp_data_p(buf,plen,PSTR("\">[刷新]</a></small></p>\n<p><a href=\""));
    // the url looks like this http://baseurl/password/command
    plen=fill_tcp_data(buf,plen,baseurl);
    plen=fill_tcp_data(buf,plen,password);
    if (on_off)
    {
        plen=fill_tcp_data_p(buf,plen,PSTR("/0\">关闭LED</a><p>"));
    }
    else
    {
        plen=fill_tcp_data_p(buf,plen,PSTR("/1\">点亮LED</a><p>"));
    }
    plen=fill_tcp_data_p(buf,plen,PSTR("</center><hr><br>****************STM32测试****************\n"));
    //plen=fill_tcp_data_p(buf,plen,PSTR("这是一个 web 测试程序"));
    
    return(plen);
}

/*
 * 函数名：Web_Server
 * 描述  ：在浏览器上创建一个web服务器，通过web里面的命令来控制开发板上的LED的亮灭。
 * 输入  ：无
 * 输出  ：无
 * 返回  ：-0 运行成功
 * 应用  ：1 在PC机的DOS界面输入： ping 192.168.1.15 (看能否ping通)
 *         2 在IE浏览器中输入：http://192.168.1.15/123456 则会出现一个网页，通过网页
 *           中的命令可以控制开发板中的LED的亮灭
 */
int Web_Server(void)
{   
	unsigned int plen, i1 = 0;
	unsigned int dat_p;
	unsigned char i = 0;
	unsigned char cmd, *buf1;
	unsigned int payloadlen = 0;
  
  /* 初始化 enc28j60 的MAC地址(物理地址),这个函数必须要调用一次 */
  enc28j60Init(mymac);

	/* PHY LED 配置,LED用来指示通信的状态 */	
  enc28j60PhyWrite(PHLCON,0x476);	
	
	/* 将enc28j60第三引脚的时钟输出改为：from 6.25MHz to 12.5MHz(本例程该引脚NC,没用到) */	
  //enc28j60clkout(2);    
  
	/* 初始化以太网 IP 层 */
	init_ip_arp_udp_tcp(mymac,myip,mywwwport);

  while(1)
  {   
			// get the next new packet:			
      plen = enc28j60PacketReceive(BUFFER_SIZE, buf);      
      
      // plen will ne unequal to zero if there is a valid packet (without crc error)			
      if(plen==0)
      {
          continue;
      }

      // arp is broadcast if unknown but a host may also
      // verify the mac address by sending it to 
      // a unicast address.		     
      if(eth_type_is_arp_and_my_ip(buf,plen))
      {
          make_arp_answer_from_request(buf);          
          continue;
      }
      
      // check if ip packets are for us:			
      if(eth_type_is_ip_and_my_ip(buf,plen)==0) 
      {
          continue;
      }      
      
      if(buf[IP_PROTO_P]==IP_PROTO_ICMP_V && buf[ICMP_TYPE_P]==ICMP_TYPE_ECHOREQUEST_V)
      {
          // a ping packet, let's send pong  DOS 下的 ping 命令包		 
          make_echo_reply_from_request(buf, plen);          
          continue;
      }

/*-----------------tcp port www start, compare only the lower byte-----------------------------------*/
	  
	  if (buf[IP_PROTO_P]==IP_PROTO_TCP_V&&buf[TCP_DST_PORT_H_P]==0&&buf[TCP_DST_PORT_L_P]==mywwwport)
      {
          if (buf[TCP_FLAGS_P] & TCP_FLAGS_SYN_V)
          {
              make_tcp_synack_from_syn(buf);
              // make_tcp_synack_from_syn does already send the syn,ack
              continue;
          }
          if (buf[TCP_FLAGS_P] & TCP_FLAGS_ACK_V)
          {
              init_len_info(buf); // init some data structures
              // we can possibly have no data, just ack:
              dat_p=get_tcp_data_pointer();
              if (dat_p==0)
              {
                  if (buf[TCP_FLAGS_P] & TCP_FLAGS_FIN_V)
                  {
                      // finack, answer with ack
                      make_tcp_ack_from_any(buf);
                  }
                  // just an ack with no data, wait for next packet
                  continue;
              }
			  
			  
			  printf("buf: fdsfasdfsdf\r\n");//, (char *)&(buf[dat_p]) + 4);
			  
			  
              if (strncmp("GET ",(char *)&(buf[dat_p]),4)!=0)
              {
                  // head, post and other methods:
                  //
                  // for possible status codes see:
                  // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
                  plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<h1>200 OK</h1>"));
                  goto SENDTCP;
              }

              if (strncmp("/ ",(char *)&(buf[dat_p+4]),2)==0)
              {
                  plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"));
                  plen=fill_tcp_data_p(buf,plen,PSTR("<p>Usage: "));
                  plen=fill_tcp_data(buf,plen,baseurl);
                  plen=fill_tcp_data_p(buf,plen,PSTR("passwords</p>"));
                  goto SENDTCP;
              }
              cmd=analyse_get_url((char *)&(buf[dat_p+5]));
              // for possible status codes see:
              // http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
              if (cmd==-1)
              {
                  plen=fill_tcp_data_p(buf,0,PSTR("HTTP/1.1 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
                  goto SENDTCP;
              }
              if (cmd==1)	           // 用户程序
              {			
                  i=1;							 // 命令 = 1
              }
              if (cmd==0)						 // 用户程序
              {									
                  i=0;							 // 命令 = 0
              }
              // if (cmd==-2) or any other value
              // just display the status:
              plen=print_webpage(buf,(i));
              
              SENDTCP:
              make_tcp_ack_from_any(buf);       // send ack for http get
              make_tcp_ack_with_data(buf,plen); // send data
              continue;
          }
      }
/*-------------------------------------- tcp port www end ---------------------------------------*/

/*--------------------- udp start, we listen on udp port 1200=0x4B0 -----------------------------*/
      if (buf[IP_PROTO_P]==IP_PROTO_UDP_V&&buf[UDP_DST_PORT_H_P]==4&&buf[UDP_DST_PORT_L_P]==0xb0)
      {
          payloadlen=	  buf[UDP_LEN_H_P];
          payloadlen=payloadlen<<8;
          payloadlen=(payloadlen+buf[UDP_LEN_L_P])-UDP_HEADER_LEN;
          //payloadlen=buf[UDP_LEN_L_P]-UDP_HEADER_LEN;
          
          ANSWER:
          //while(1){
          for(i1=0; i1<payloadlen; i1++) buf1[i1]=buf[UDP_DATA_P+i1];
          
          //make_udp_reply_from_request(buf,str,strlen(str),myudpport);
          make_udp_reply_from_request(buf,buf1,payloadlen,myudpport);          
          //}
      }
/*----------------------------------------udp end -----------------------------------------------*/
  }
          return (0);
}
/******************* (C) COPYRIGHT 2011 野火嵌入式开发工作室 *****END OF FILE****/
