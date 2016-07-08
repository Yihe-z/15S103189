#include "p2pServer.h"
#include <sstream>
#include <vector>
using std::vector;
using std::string;
using std::istringstream;
using std::ostringstream;

//���Ҽ�¼ �ҵ�����1 ���ļ������޵�ַ ����2�����ļ������޵�ַ ����3
int Server::searchRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, vector<string>::iterator &iterCols)
{
	/*----------------����--------------*/
	AcquireSRWLockShared(&g_srwLock);
	//���Ҽ�¼�����������ڵ�ַ,���ɹ�������true�����򷵻�false
	Records::iterator iterRecordsBegin = g_Records.begin();
	Records::iterator iterRecordsEnd = g_Records.end();
	while (iterRecordsBegin != iterRecordsEnd)
	{
		if (iterRecordsBegin->sFileName == sFileName)
		{
			vector<string>::iterator iterAddrBegin = iterRecordsBegin->vecAddrs.begin();
			vector<string>::iterator iterAddrEnd = iterRecordsBegin->vecAddrs.end();
			while (iterAddrBegin != iterAddrEnd)
			{
				if (*iterAddrBegin == sAddr)//�ҵ���¼
				{
					iterRows = iterRecordsBegin;
					iterCols = iterAddrBegin;
					ReleaseSRWLockShared(&g_srwLock);
					return 1;
				}
				++iterAddrBegin;
			}
			//���ļ������޵�ַ
			iterRows = iterRecordsBegin;
			iterCols = iterAddrBegin;
			ReleaseSRWLockShared(&g_srwLock);
			return 2;
		}
		++iterRecordsBegin;
	}
	//���ļ������޵�ַ
	iterRows = iterRecordsBegin;
	ReleaseSRWLockShared(&g_srwLock);
	return 3;
}
//���������б�
void Server::listFiles(SOCKET sock)
{
	/*-----------����---------------*/
	AcquireSRWLockShared(&g_srwLock);

	Records::iterator iterRecordsBegin = g_Records.begin();
	Records::iterator iterRecordsEnd = g_Records.end();
	vector<string> sFiles;
	char buf[BUFF_SIZE];
	while (iterRecordsBegin != iterRecordsEnd)
	{
		sFiles.push_back(iterRecordsBegin->sFileName);
		++iterRecordsBegin;
	}

	ReleaseSRWLockShared(&g_srwLock);
	/*---------------------�������-------------*/
	if (sFiles.empty())
	{
		string sInfo = "ERROR SORRY HAS NO FILES";
		strncpy(buf, sInfo.c_str(), sInfo.size());
		send(sock, buf, sInfo.size(), 0);
	}
	else
	{
		vector<string>::iterator iterBegin = sFiles.begin();
		vector<string>::iterator iterEnd = sFiles.end();
		while (iterBegin != iterEnd)
		{
			strncpy(buf, iterBegin->c_str(), iterBegin->size());
			send(sock, buf, iterBegin->size(), 0);
			//�յ�ȷ��
			Common::recvMsg(sock);
			++iterBegin;
		}
		string sInfo = "OK";
		strncpy(buf, sInfo.c_str(), sInfo.size());
		send(sock, buf, sInfo.size(), 0);
	}
}
//���ݲ��ҽ�����ص�ַ
void Server::requestFile(SOCKET sock, const string &sFileName)
{
	string sAddr = "0.0.0.0 0000";
	Records::iterator iterRows;
	vector<string>::iterator iterCols;
	int iRes = searchRecord(sFileName, sAddr, iterRows, iterCols);
	char buf[BUFF_SIZE];
	if (iRes == 2)//�ҵ��ļ�
	{
		sAddr = iterRows->vecAddrs[0];
	}
	else//û�ҵ��ļ�
	{
		sAddr = string("ERROR NOT FOUND SUCH FILE");
	}
	strncpy(buf, sAddr.c_str(), sAddr.size());
	send(sock, buf, sAddr.size(), 0);
	//����ҵ�������ǰaddr�ŵ���� �������
	if (iRes == 2)
	{
		/*-------------------����--------------------------*/
		AcquireSRWLockExclusive(&g_srwLock);

		iterRows->vecAddrs.erase(iterRows->vecAddrs.begin());
		iterRows->vecAddrs.push_back(sAddr);

		ReleaseSRWLockExclusive(&g_srwLock);
		/*-------------------�������--------------------------*/
	}
}
//���ݲ��ҽ������Ӽ�¼
void Server::addRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, const int &iSearchRes)
{
	/*----------------����------------------*/
	AcquireSRWLockExclusive(&g_srwLock);

	//���ļ��������
	if (iSearchRes == 2)
	{
		iterRows->vecAddrs.push_back(sAddr);
	}
	else if (iSearchRes == 3)//���ļ��������
	{
		vector<string> vecAddr(1, sAddr);
		Record oneRec = { sFileName, vecAddr };
		g_Records.push_back(oneRec);
	}

	ReleaseSRWLockExclusive(&g_srwLock);
}
//ɾ����¼
void Server::deleteRecord(Records::iterator &iterRows, vector<string>::iterator &iterCols)
{
	/*------------------����------------------*/
	AcquireSRWLockExclusive(&g_srwLock);

	iterRows->vecAddrs.erase(iterCols);
	if (iterRows->vecAddrs.empty())
	{
		g_Records.erase(iterRows);
	}

	ReleaseSRWLockExclusive(&g_srwLock);
}
//�Լ�¼����
void Server::modifyRecord(SOCKET sock, const string &sFileName, const string &sAddr, const string &opt)
{
	Records::iterator iterRows;
	vector<string>::iterator iterCols;
	char buf[BUFF_SIZE];
	//����
	int iRes = searchRecord(sFileName, sAddr, iterRows, iterCols);
	if (opt == "ADD")//�������
	{
		if (iRes == 1)//�Ѿ����ļ������ش���
		{
			string sError("ERROR YOU HAVE UPLOAD BEFORE");
			strncpy(buf, sError.c_str(), sError.size());
			send(sock, buf, sError.size(), 0);
		}
		else//�������
		{
			addRecord(sFileName, sAddr, iterRows, iRes);
			string sInfo("OK");
			strncpy(buf, sInfo.c_str(), sInfo.size());
			send(sock, buf, sInfo.size(), 0);
		}
	}
	else if (opt == "DELETE")
	{
		if (iRes != 1)//û�ҵ���¼
		{
			string sError("ERROR NOT SUCH FILE");
			strncpy(buf, sError.c_str(), sError.size());
			send(sock, buf, sError.size(), 0);
		}
		else
		{
			deleteRecord(iterRows, iterCols);
			string sInfo("OK");
			strncpy(buf, sInfo.c_str(), sInfo.size());
			send(sock, buf, sInfo.size(), 0);
		}
	}
	else//��ѡ��
	{

	}
}
//ÿ�����������߳� ����
void Server::tcpThreadTask(Server *pServer)
{
	SOCKET sock = pServer->sockAndAddr.sock;
	string sClientIp = pServer->sockAndAddr.sIP;
	string sClientPort = pServer->sockAndAddr.sPort;
	ReleaseSemaphore(pServer->g_hSockAndAddr, 1, NULL);
	printf("�̺߳ţ�%d �ͻ��� ip �� �˿�Ϊ%s:%s\n",GetCurrentThreadId(), sClientIp.c_str(), sClientPort.c_str());
	/*������ʽ
	���ӽ�����CONNECT ACCEPT
	���
	ADD <FILE_NAME>  OK | ERROR + INFO
	DELETE <FILENAME> OK | ERROR +INFO
	LIST (SHOW ALL FILES) OK | ERROR
	REQUEST <FILENAME> ADDR(IP+PORT) | ERROR + INFO
	QUIT
	*/
	char buf[BUFF_SIZE];
	int iDataSize = 0;
	if ((iDataSize = recv(sock, buf, BUFF_SIZE, 0)) == SOCKET_ERROR)
	{
		Common::errexit("recvfrom: error %d\n", GetLastError());
	}
	string sConn;
	buf[iDataSize] = '\0';
	istringstream sConnCommand(buf);
	sConnCommand >> sConn;
	if (sConn != "CONNECT")//�Ƿ�����
	{
		//���� �Ƿ�����
		string sInfo("ILLEGAL CONNECT");
		strncpy(buf, sInfo.c_str(), sInfo.size());
		send(sock, buf, sInfo.size(), 0);
		ReleaseSemaphore(pServer->g_hThreadNumber, 1, NULL);
		return;
	}
	else
	{
		//����ACCEPT
		string sAcc("ACCEPT");
		strncpy(buf, sAcc.c_str(), sAcc.size());
		send(sock, buf, sAcc.size(), 0);
	}
	//���ն˿ں�
	sClientPort = Common::recvMsg(sock);
	Common::sendMsg(sock, "PORT_ACCEPT");
	printf("�ͻ��˶�Ӧ�� �������˿ں�Ϊ %s\n", sClientPort.c_str());
	//��ʼ���� ����
	while (true)
	{
		if ((iDataSize = recv(sock, buf, BUFF_SIZE, 0)) == SOCKET_ERROR)
		{
			Common::errexit("recvfrom: error %d\n", GetLastError());
		}
		string sComArg1;
		buf[iDataSize] = '\0';
		istringstream sCommand(buf);
		sCommand >> sComArg1;
		if (sComArg1 == "ADD")
		{
			string sFileName;
			sCommand >> sFileName;
			pServer->modifyRecord(sock, sFileName, sClientIp + " " + sClientPort, "ADD");
		}
		else if (sComArg1 == "DELETE")
		{
			string sFileName;
			sCommand >> sFileName;
			pServer->modifyRecord(sock, sFileName, sClientIp + " " + sClientPort, "DELETE");
		}
		else if (sComArg1 == "LIST")
		{
			pServer->listFiles(sock);
		}
		else if (sComArg1 == "REQUEST")
		{
			string sFileName;
			sCommand >> sFileName;
			pServer->requestFile(sock, sFileName);
		}
		else if (sComArg1 == "QUIT")
		{
			
			closesocket(sock);
			break;
		}
		else//��������
		{
			string sInfo = "ERROR COMMAND";
			strncpy(buf, sInfo.c_str(), sInfo.size());
			send(sock, buf, sInfo.size(), 0);
		}
	}
	printf("�ͷ��߳� %d ����\n",GetCurrentThreadId());
	ReleaseSemaphore(pServer->g_hThreadNumber, 1, NULL);
}
//���������߳� ���� ��ʼ�� �� ����������
Server::Server(const char *service)
{
	//��ʼ���ź������д��
	g_hThreadNumber = CreateSemaphore(NULL, MAX_THREAD_NUM, MAX_THREAD_NUM, NULL);
	g_hSockAndAddr = CreateSemaphore(NULL, 0, 1, NULL);
	InitializeSRWLock(&g_srwLock);
	WSADATA wsadata;
	if (WSAStartup(WSVERS, &wsadata) != 0)
		Common::errexit("WSAStartup failed\n");

	//put master tcp and udp into the fdset
	tcpSock = Common::passiveTCP(service);//������listen����Ҫaccept
	
}
void Server::run()
{
	
	SOCKET newTcpSock;
	//start select *******************************
	struct sockaddr_in addrIn;
	int iAddrSize;
	//SockAndAddr sockAndAddr;
	printf("start listening ... \n");
	while (true)
	{
		//printf("start listening ... \n");
		//�ȴ������߳�
		WaitForSingleObject(g_hThreadNumber, INFINITE);

		iAddrSize = sizeof(struct sockaddr_in);
		if ((newTcpSock = accept(tcpSock, (struct sockaddr*)&addrIn, &iAddrSize)) != SOCKET_ERROR)
		{
			string sClientIp = (inet_ntoa(*(in_addr*)(&addrIn.sin_addr.s_addr)));//��ip��ַ�������ʽת��Ϊ��ָ�ʽ
			ostringstream os;
			os << ntohs(addrIn.sin_port);//���˿ڵ������ʽװ��Ϊshort
			string sClientPort = os.str();
			sockAndAddr = { newTcpSock, sClientIp, sClientPort };
			if ((_beginthread((ThreadFunType)tcpThreadTask, 0, this)) < 0)
			{
				Common::errexit("_beginthread: %s\n", strerror(errno));
			}
			else
			{
				WaitForSingleObject(g_hSockAndAddr, INFINITE);
				printf("���ܵ�����������\n");
			}
		}
		else
		{
			Common::errexit("accept failed:error number��%d\n", GetLastError());
		}

	}
	closesocket(tcpSock);
	//**************************
	WSACleanup();
	//�ر��ź���
	CloseHandle(g_hSockAndAddr);
	CloseHandle(g_hThreadNumber);
}
