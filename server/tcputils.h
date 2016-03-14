#ifndef __TCPUTILS_H__
	#define __TCPUTILS_H__
	
#define MAXCONNECT  10	

#define SER_PORT    8610
#define CLI_PORT    8612

static inline void close_sock(int sock)
{
	close(sock);
}

extern int open_ser_sock (int port);
extern int open_cli_sock(char *ip_addr, int port);
extern int read_sock(int sock, unsigned char *buf, int len);
extern int write_sock(int sock, unsigned char *buf, int len);
	
#endif 	//__TCPUTILS_H__
