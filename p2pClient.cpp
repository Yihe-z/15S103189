#include "p2pClient.h"
#include <iostream>
#include <sstream>
using std::cout;
using std::cin;
using std::endl;
using std::string;
using std::istringstream;


//p2p �ͻ������еķ�����

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
	//���ļ�
	FILE *fp = fopen(sFilePath.c_str(), "rb");
	if (fp == NULL)
	{
		Common::sendMsg(sock, "FILE NOT EXIST");
		return;
	}
	//�����ļ�
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
	SOCKET sock = Common::passiveTCP(service);//������listen����Ҫaccept
	SOCKET newsock;
	while ((newsock = accept(sock, 0, 0)) != SOCKET_ERROR)
	{
		pClient->servSendFile(newsock);
		closesocket(newsock);
	}
	closesocket(sock);
	return;
}
//�����ļ�
void Client::getFileTcp(const char *host, const char *service, const char *filePath)
{
	char buf[BUFF_SIZE]; /* buffer for one line of text */
	//��������
	SOCKET s; /* socket descriptor */
	s = Common::connectTCP(host, service);
	Common::sendMsg(s, "HELLO");
	string sMsg = Common::recvMsg(s);
	if (sMsg != "ACCEPT")
	{
		cout << "�޷�����" << endl;
		return;
	}
	Common::sendMsg(s, filePath);
	//���ļ���ʼ����
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
//������������
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
		cout << "�����ѽ��� ..." << endl;
	}
	else
	{
		cout << "����ʧ�� ..." << endl;
		system("pause");
		return;
	}
	//���͵�ǰ�������˿�
	Common::sendMsg(sock, string(localService));
	//����ȷ��
	Common::recvMsg(sock);
	//getchar();
	while (true)
	{
		//��������
		cout << "what do you want to do ?" << endl;
		string sInput = "";
		while (sInput.empty())
		{
			getline(cin, sInput);
		}
		//��������
		Common::sendMsg(sock, sInput);
		//��������
		sMsg = Common::recvMsg(sock);
		//�������
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
				cout << "��ǰ�ļ���������Щ��" << endl;
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
			else//�ҵ��ļ���ַ
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
		else//������������
		{
			cout << sMsg << endl;
		}
	}
}
//p2p �ͻ���
void Client::run()
{
	WSADATA wsadata;
	if (WSAStartup(WSVERS, &wsadata) != 0)
		Common::errexit("WSAStartup failed\n");
	HANDLE threads[2];
	//��������������
	threads[0] = (HANDLE)_beginthread((FunType)p2pClientServer, 0, (void*)sLocalPort.c_str());
	//�����ͻ��˽���
	//IpAndPort ipAndPort = { string(host), string(service), string(localService) };
	threads[1] = (HANDLE)_beginthread((FunType)connMasterTask, 0, this);

	WaitForMultipleObjects(2, threads, FALSE, INFINITE);
	WSACleanup();
}
