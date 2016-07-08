#ifndef _H_COMMON
#define _H_COMMON
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <WinSock2.h>
#include <process.h>
#define WSVERS MAKEWORD(2,0)
#define BUFF_SIZE 1024
#define QLEN 32
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif /* INADDR_NONE */
class Common{
private:
	//��������socket
	static SOCKET connectsock(const char* host, const char* service, const char *transport);
	static SOCKET passsock(const char* service, const char *transport, int qlen);
public:
	//����socket
	static SOCKET connectTCP(const char* host, const char* service);
	static SOCKET passiveTCP(const char *service);
	//�շ�����
	static void sendMsg(SOCKET sock, const std::string &sMsg);
	static std::string recvMsg(SOCKET sock);
	//������
	static void errexit(const char *format, ...);
};
#endif
