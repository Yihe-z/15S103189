/* errexit.cpp - errexit */
#include "common.h"
#include <stdarg.h>
using std::string;
/*----------------------------------------------------------
* errexit - print an error message and exit
*----------------------------------------------------------
*/
/*VARARGS1*/
SOCKET Common::connectsock(const char* host, const char* service,const char *transport)
{
	struct hostent *phe; /* pointer to host information entry */
	struct servent *pse; /* pointer to service information entry */
	struct protoent *ppe; /* pointer to protocol information entry */
	struct sockaddr_in sin;/* an Internet endpoint address */
	int s, type; /* socket descriptor and socket type */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	/* Map service name to port number */
	if (pse = getservbyname(service, transport))
		sin.sin_port = pse->s_port;
	else if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
	{
		errexit("can't get \"%s\" service entry\n", service);
	}
	/* Map host name to IP address, allowing for dotted decimal */
	if (phe = gethostbyname(host))
	{
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE)
	{
		errexit("can't get \"%s\" host entry\n", host);
	}
	/* Map protocol name to protocol number */
	ppe = getprotobyname("tcp");
	if ((ppe = getprotobyname(transport)) == 0)
	{
		//errexit("can't get \"%s\" protocol entry\n", transport);
	}
	/* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;
	/* Allocate a socket */
	s = socket(PF_INET, type, 0);
	if (s == INVALID_SOCKET)
		errexit("can't create socket: %d\n", GetLastError());
	/* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR)
		errexit("can't connect to %s.%s: %d\n", host, service,
		GetLastError());
	//puts("ok");
	return s;
}
SOCKET Common::passsock(const char* eservice,const char *transport,int qlen)
{
	string sService(eservice);
	const char *service = sService.c_str();
	struct servent *pse; /* pointer to service information entry */
	struct protoent *ppe; /* pointer to protocol information entry */
	struct sockaddr_in sin;/* an Internet endpoint address */
	SOCKET s; /* socket descriptor */
	int type; /* socket type (SOCK_STREAM, SOCK_DGRAM)*/
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	/* Map service name to port number */
	/*if (pse = getservbyname(service, transport))
		sin.sin_port = (u_short)pse->s_port;
	else*/ if ((sin.sin_port = htons((u_short)atoi(service))) == 0)
		errexit("can't get \"%s\" service entry\n", service);
	/* Map protocol name to protocol number */
	if ((ppe = getprotobyname(transport)) == 0)
		errexit("can't get \"%s\" protocol entry\n", transport);
	/* Use protocol to choose a socket type */
	if (strcmp(transport, "udp") == 0)
		type = SOCK_DGRAM;
	else
		type = SOCK_STREAM;
	/* Allocate a socket */
	s = socket(PF_INET, type, ppe->p_proto);
	if (s == INVALID_SOCKET)
		errexit("can't create socket: %d\n", GetLastError());
	/* Bind the socket */
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) == SOCKET_ERROR)
		errexit("can't bind to %s port: %d\n", service,
		GetLastError());
	if (type == SOCK_STREAM && listen(s, qlen) == SOCKET_ERROR)
		errexit("can't listen on %s port: %d\n", service,
		GetLastError());
	return s;
}
SOCKET Common::connectTCP(const char* host, const char* service)
{
	return connectsock(host, service, "tcp");
}
SOCKET Common::passiveTCP(const char *service)
{
	return passsock(service, "tcp", QLEN);
}
void Common::sendMsg(SOCKET sock, const std::string &sMsg)
{
	static char buf[BUFF_SIZE];
	strncpy(buf, sMsg.c_str(), sMsg.size());
	send(sock, buf, sMsg.size(), 0);
}
std::string Common::recvMsg(SOCKET sock)
{
	static char buf[BUFF_SIZE];
	int iLength = recv(sock, buf, BUFF_SIZE, 0);
	if (iLength > 0)
	{
		buf[iLength] = '\0';
		return std::string(buf);
	}
	else
	{
		return "";
	}
}
void Common::errexit(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    WSACleanup();
	system("pause");
    exit(1);
}
