#ifndef _H_P2PCLIENT
#define _H_P2PCLIENT
#include "common.h"
using std::string;
class Client{
private:
	//���Ͷ���
	struct IpAndPort
	{
		string ip;
		string port;
		string localport;
	};
	typedef void (*FunType)(void *);
	//˽�к�������
	void servSendFile(SOCKET sock);
	void getFileTcp(const char *host, const char *service, const char *filePath);
	static void p2pClientServer(Client *);
	static void connMasterTask(Client *);
public:
	Client(const char * host, const char * port, const char *localService) :sHost(host), sPort(port), sLocalPort(localService){}
	void run();
private:
	string sHost;
	string sPort;
	string sLocalPort;
};
#endif