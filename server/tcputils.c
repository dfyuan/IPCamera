#include "stdinc.h"
#include "utils.h"

#include "tcputils.h"

static int set_sockaddr(struct sockaddr_in *sock_addr, char *ip_addr, int port)
{  
	int siz = 0;
	
	if(ip_addr){
		siz = strlen(ip_addr);
		if(siz < 7 || siz > 15) {
			printf("%s(%d): str_addr is illegal!\n", __func__, __LINE__);
			return -1;
		}
		sock_addr->sin_addr.s_addr = inet_addr(ip_addr);
	} else {
		sock_addr->sin_addr.s_addr = INADDR_ANY;
	}
		
	sock_addr->sin_family = AF_INET;
  	sock_addr->sin_port = htons (port);
   	bzero(&(sock_addr->sin_zero), 8);
	
	return 0;
}

int open_ser_sock(int port)
{
	struct sockaddr_in ser_addr;
	int ser_sock;
	int on = TRUE;
	
	if (-1 == (ser_sock=socket(AF_INET, SOCK_STREAM, 0)))
		ERR_PRINT_RET(-1);
		
	if (-1 == setsockopt(ser_sock, SOL_SOCKET, SO_REUSEADDR, 
	                      &on, sizeof(int)))
		ERR_PRINT_RET(-1);
		
	if (-1 == set_sockaddr(&ser_addr, NULL, port))
		ERR_PRINT_RET(-1);
	
	if (-1 == bind(ser_sock, (struct sockaddr*)&ser_addr, 
	                sizeof(struct sockaddr)))
		ERR_PRINT_RET(-1);
		
	if (-1 == listen(ser_sock, MAXCONNECT))
		ERR_PRINT_RET(-1);
		
	return ser_sock;
}

int open_cli_sock(char *ip_addr, int port)
{
	struct sockaddr_in ser_addr;
	int cli_sock;

	if (-1 == (cli_sock = socket(AF_INET, SOCK_STREAM, 0)))
		ERR_PRINT_RET(-1);

	set_sockaddr(&ser_addr, ip_addr, port);

	if (-1 == connect(cli_sock, (struct sockaddr*)&ser_addr,
	             sizeof(struct sockaddr)))
		ERR_PRINT_RET(-1);
		
	return cli_sock;
}

int read_sock(int sock, unsigned char *buf, int len)
{
	int bhr = -1;
	unsigned char *pbuf =buf;
	int rlen = len;       //rlen -- 未读字节数
	
	do {
		bhr = read(sock, pbuf, rlen);
		if (bhr > 0) {
			pbuf += bhr;
			rlen -= bhr;
		} else if (-1 == bhr) {
			ERR_PRINT_RET(-1);
		}
	} while (rlen > 0);
	
	return rlen;
}

int write_sock(int sock, unsigned char *buf, int len)
{
	int bhw = -1;
	unsigned char *pbuf =buf;
	int wlen = len;       //rlen -- 未读字节数
	
	do {
		bhw = write(sock, pbuf, wlen);
		if (bhw > 0) {
			pbuf += bhw;
			wlen -= bhw;
		} else if (-1 == bhw) {
			ERR_PRINT_RET(-1);
		}
	} while (wlen > 0);
	
	return wlen;
}






