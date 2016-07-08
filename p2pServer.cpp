#include "p2pServer.h"
#include <sstream>
#include <vector>
using std::vector;
using std::string;
using std::istringstream;
using std::ostringstream;

//查找记录 找到返回1 有文件名，无地址 返回2，无文件名，无地址 返回3
int Server::searchRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, vector<string>::iterator &iterCols)
{
	/*----------------共享--------------*/
	AcquireSRWLockShared(&g_srwLock);
	//查找记录，并返回所在地址,若成功，返回true，否则返回false
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
				if (*iterAddrBegin == sAddr)//找到记录
				{
					iterRows = iterRecordsBegin;
					iterCols = iterAddrBegin;
					ReleaseSRWLockShared(&g_srwLock);
					return 1;
				}
				++iterAddrBegin;
			}
			//有文件名，无地址
			iterRows = iterRecordsBegin;
			iterCols = iterAddrBegin;
			ReleaseSRWLockShared(&g_srwLock);
			return 2;
		}
		++iterRecordsBegin;
	}
	//无文件名，无地址
	iterRows = iterRecordsBegin;
	ReleaseSRWLockShared(&g_srwLock);
	return 3;
}
//返回整个列表
void Server::listFiles(SOCKET sock)
{
	/*-----------共享---------------*/
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
	/*---------------------共享结束-------------*/
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
			//收到确认
			Common::recvMsg(sock);
			++iterBegin;
		}
		string sInfo = "OK";
		strncpy(buf, sInfo.c_str(), sInfo.size());
		send(sock, buf, sInfo.size(), 0);
	}
}
//根据查找结果返回地址
void Server::requestFile(SOCKET sock, const string &sFileName)
{
	string sAddr = "0.0.0.0 0000";
	Records::iterator iterRows;
	vector<string>::iterator iterCols;
	int iRes = searchRecord(sFileName, sAddr, iterRows, iterCols);
	char buf[BUFF_SIZE];
	if (iRes == 2)//找到文件
	{
		sAddr = iterRows->vecAddrs[0];
	}
	else//没找到文件
	{
		sAddr = string("ERROR NOT FOUND SUCH FILE");
	}
	strncpy(buf, sAddr.c_str(), sAddr.size());
	send(sock, buf, sAddr.size(), 0);
	//如果找到，将当前addr放到最后 互斥访问
	if (iRes == 2)
	{
		/*-------------------互斥--------------------------*/
		AcquireSRWLockExclusive(&g_srwLock);

		iterRows->vecAddrs.erase(iterRows->vecAddrs.begin());
		iterRows->vecAddrs.push_back(sAddr);

		ReleaseSRWLockExclusive(&g_srwLock);
		/*-------------------互斥结束--------------------------*/
	}
}
//根据查找结果，添加记录
void Server::addRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, const int &iSearchRes)
{
	/*----------------互斥------------------*/
	AcquireSRWLockExclusive(&g_srwLock);

	//有文件名的添加
	if (iSearchRes == 2)
	{
		iterRows->vecAddrs.push_back(sAddr);
	}
	else if (iSearchRes == 3)//无文件名的添加
	{
		vector<string> vecAddr(1, sAddr);
		Record oneRec = { sFileName, vecAddr };
		g_Records.push_back(oneRec);
	}

	ReleaseSRWLockExclusive(&g_srwLock);
}
//删除记录
void Server::deleteRecord(Records::iterator &iterRows, vector<string>::iterator &iterCols)
{
	/*------------------互斥------------------*/
	AcquireSRWLockExclusive(&g_srwLock);

	iterRows->vecAddrs.erase(iterCols);
	if (iterRows->vecAddrs.empty())
	{
		g_Records.erase(iterRows);
	}

	ReleaseSRWLockExclusive(&g_srwLock);
}
//对记录更新
void Server::modifyRecord(SOCKET sock, const string &sFileName, const string &sAddr, const string &opt)
{
	Records::iterator iterRows;
	vector<string>::iterator iterCols;
	char buf[BUFF_SIZE];
	//查找
	int iRes = searchRecord(sFileName, sAddr, iterRows, iterCols);
	if (opt == "ADD")//添加命令
	{
		if (iRes == 1)//已经有文件，返回错误
		{
			string sError("ERROR YOU HAVE UPLOAD BEFORE");
			strncpy(buf, sError.c_str(), sError.size());
			send(sock, buf, sError.size(), 0);
		}
		else//可以添加
		{
			addRecord(sFileName, sAddr, iterRows, iRes);
			string sInfo("OK");
			strncpy(buf, sInfo.c_str(), sInfo.size());
			send(sock, buf, sInfo.size(), 0);
		}
	}
	else if (opt == "DELETE")
	{
		if (iRes != 1)//没找到记录
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
	else//无选项
	{

	}
}
//每个服务器子线程 任务
void Server::tcpThreadTask(Server *pServer)
{
	SOCKET sock = pServer->sockAndAddr.sock;
	string sClientIp = pServer->sockAndAddr.sIP;
	string sClientPort = pServer->sockAndAddr.sPort;
	ReleaseSemaphore(pServer->g_hSockAndAddr, 1, NULL);
	printf("线程号：%d 客户端 ip 和 端口为%s:%s\n",GetCurrentThreadId(), sClientIp.c_str(), sClientPort.c_str());
	/*交互格式
	连接建立：CONNECT ACCEPT
	命令：
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
	if (sConn != "CONNECT")//非法连接
	{
		//返回 非法连接
		string sInfo("ILLEGAL CONNECT");
		strncpy(buf, sInfo.c_str(), sInfo.size());
		send(sock, buf, sInfo.size(), 0);
		ReleaseSemaphore(pServer->g_hThreadNumber, 1, NULL);
		return;
	}
	else
	{
		//返回ACCEPT
		string sAcc("ACCEPT");
		strncpy(buf, sAcc.c_str(), sAcc.size());
		send(sock, buf, sAcc.size(), 0);
	}
	//接收端口号
	sClientPort = Common::recvMsg(sock);
	Common::sendMsg(sock, "PORT_ACCEPT");
	printf("客户端对应的 服务器端口号为 %s\n", sClientPort.c_str());
	//开始接受 命令
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
		else//错误命令
		{
			string sInfo = "ERROR COMMAND";
			strncpy(buf, sInfo.c_str(), sInfo.size());
			send(sock, buf, sInfo.size(), 0);
		}
	}
	printf("释放线程 %d 连接\n",GetCurrentThreadId());
	ReleaseSemaphore(pServer->g_hThreadNumber, 1, NULL);
}
//服务器主线程 负责 初始化 及 监听新请求
Server::Server(const char *service)
{
	//初始化信号量与读写锁
	g_hThreadNumber = CreateSemaphore(NULL, MAX_THREAD_NUM, MAX_THREAD_NUM, NULL);
	g_hSockAndAddr = CreateSemaphore(NULL, 0, 1, NULL);
	InitializeSRWLock(&g_srwLock);
	WSADATA wsadata;
	if (WSAStartup(WSVERS, &wsadata) != 0)
		Common::errexit("WSAStartup failed\n");

	//put master tcp and udp into the fdset
	tcpSock = Common::passiveTCP(service);//已启动listen，需要accept
	
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
		//等待可用线程
		WaitForSingleObject(g_hThreadNumber, INFINITE);

		iAddrSize = sizeof(struct sockaddr_in);
		if ((newTcpSock = accept(tcpSock, (struct sockaddr*)&addrIn, &iAddrSize)) != SOCKET_ERROR)
		{
			string sClientIp = (inet_ntoa(*(in_addr*)(&addrIn.sin_addr.s_addr)));//将ip地址的网络格式转换为点分格式
			ostringstream os;
			os << ntohs(addrIn.sin_port);//将端口的网络格式装换为short
			string sClientPort = os.str();
			sockAndAddr = { newTcpSock, sClientIp, sClientPort };
			if ((_beginthread((ThreadFunType)tcpThreadTask, 0, this)) < 0)
			{
				Common::errexit("_beginthread: %s\n", strerror(errno));
			}
			else
			{
				WaitForSingleObject(g_hSockAndAddr, INFINITE);
				printf("接受到新连接请求\n");
			}
		}
		else
		{
			Common::errexit("accept failed:error number：%d\n", GetLastError());
		}

	}
	closesocket(tcpSock);
	//**************************
	WSACleanup();
	//关闭信号量
	CloseHandle(g_hSockAndAddr);
	CloseHandle(g_hThreadNumber);
}
