#include "p2pClient.h"
#include <iostream>
#include <sstream>
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::istringstream;


//p2p 客户端运行的服务器

void Client::servSendFile(SOCKET sock)
{
	char buf[BUFF_SIZE];
	string sMsg = Common::recvMsg(sock);
	if (sMsg != "HELLO")
	{
		Common::sendMsg(sock, "ILLEGAL");
		return;
	}
	Common::sendMsg(sock, "ACCEPT");
	string sFilePath = Common::recvMsg(sock);
	//打开文件
	FILE *fp = fopen(sFilePath.c_str(), "rb");
	if (fp == NULL)
	{
		Common::sendMsg(sock, "FILE NOT EXIST");
		return;
	}
	//传输文件
	int length;
	while ((length = fread(buf, 1, BUFF_SIZE, fp)) > 0)
	{
		send(sock, buf, length, 0);
		Common::recvMsg(sock);
	}
	fclose(fp);
}
//tcp part
void Client::p2pClientServer(Client *pClient)
{
	const char *service = pClient->sLocalPort.c_str();
	SOCKET sock = Common::passiveTCP(service);//已启动listen，需要accept
	SOCKET newsock;
	while ((newsock = accept(sock, 0, 0)) != SOCKET_ERROR)
	{
		pClient->servSendFile(newsock);
		closesocket(newsock);
	}
	closesocket(sock);
	return;
}
//下载文件
void Client::getFileTcp(const char *host, const char *service, const char *filePath)
{
	char buf[BUFF_SIZE]; /* buffer for one line of text */
	//建立连接
	SOCKET s; /* socket descriptor */
	s = Common::connectTCP(host, service);
	Common::sendMsg(s, "HELLO");
	string sMsg = Common::recvMsg(s);
	if (sMsg != "ACCEPT")
	{
		cout << "无法连接" << endl;
		return;
	}
	Common::sendMsg(s, filePath);
	//打开文件开始接收
	FILE *fp = fopen(filePath, "wb");
	if (fp == NULL)
	{
		Common::errexit("open file error\n");
	}
	puts("start recvive file...");
	int length; /* recv character count */
	while ((length = recv(s, buf, BUFF_SIZE, 0)) > 0)
	{
		if (fwrite(buf, 1, length, fp) <= 0)
		{
			fclose(fp);
			Common::errexit("file write error\n");
		}
		else
		{
			Common::sendMsg(s, "ok");
		}
	}
	fclose(fp);
	printf("transfer complete\n");
	closesocket(s);
}
//连接主服务器
void Client::connMasterTask(Client *pClient)
{
	const char *host = pClient->sHost.c_str();
	const char *service = pClient->sPort.c_str();
	const char *localService = pClient->sLocalPort.c_str();
	SOCKET sock = Common::connectTCP(host, service);
	//char buf[BUFF_SIZE];
	Common::sendMsg(sock, "CONNECT");
	string sMsg = Common::recvMsg(sock);
	if (sMsg == "ACCEPT")
	{
		cout << "连接已建立 ..." << endl;
	}
	else
	{
		cout << "连接失败 ..." << endl;
		system("pause");
		return;
	}
	//发送当前服务器端口
	Common::sendMsg(sock, string(localService));
	//接收确认
	Common::recvMsg(sock);
	//getchar();
	while (true)
	{
		//输入命令
		cout << "what do you want to do ?" << endl;
		string sInput = "";
		while (sInput.empty())
		{
			getline(cin, sInput);
		}
		//发送命令
		Common::sendMsg(sock, sInput);
		//接受命令
		sMsg = Common::recvMsg(sock);
		//分析结果
		istringstream is(sInput);
		string sCom;
		is >> sCom;
		//add delete list request quit
		if (sCom == "ADD" || sCom == "DELETE")
		{
			cout << sMsg << endl;
		}
		else if (sCom == "LIST")
		{
			is.clear();
			is.str(sMsg);
			is >> sCom;
			if (sCom == "ERROR")
			{
				cout << sMsg << endl;
				continue;
			}
			else
			{
				cout << "当前文件有以下这些：" << endl;
				while (sMsg != "OK")
				{
					cout << sMsg << endl;
					Common::sendMsg(sock, "ok");
					sMsg = Common::recvMsg(sock);
				}
			}
		}
		else if (sCom == "REQUEST")//
		{
			is.clear();
			is.str(sMsg);
			is >> sCom;
			if (sCom == "ERROR")
			{
				cout << sMsg << endl;
				continue;
			}
			else//找到文件地址
			{
				string sClientIP = sCom;
				string sClientPort;
				is >> sClientPort;
				string sFilePath;
				is.clear();
				is.str(sInput);
				is >> sFilePath;
				is >> sFilePath;
				pClient->getFileTcp(sClientIP.c_str(), sClientPort.c_str(), sFilePath.c_str());
			}
		}
		else if (sCom == "QUIT")
		{
			closesocket(sock);
			break;
		}
		else//其他错误命令
		{
			cout << sMsg << endl;
		}
	}
}
//p2p 客户端
void Client::run()
{
	WSADATA wsadata;
	if (WSAStartup(WSVERS, &wsadata) != 0)
		Common::errexit("WSAStartup failed\n");
	HANDLE threads[2];
	//开启服务器进程
	threads[0] = (HANDLE)_beginthread((FunType)p2pClientServer, 0, (void*)sLocalPort.c_str());
	//开启客户端进程
	//IpAndPort ipAndPort = { string(host), string(service), string(localService) };
	threads[1] = (HANDLE)_beginthread((FunType)connMasterTask, 0, this);

	WaitForMultipleObjects(2, threads, FALSE, INFINITE);
	WSACleanup();
}
