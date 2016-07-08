#ifndef _H_P2PSERVER
#define _H_P2PSERVER
#include "common.h"
#include <vector>
using std::string;
using std::vector;
#define MAX_THREAD_NUM 64 
class Server{
private:
	typedef void(*ThreadFunType)(void*);
	//传递线程时传递的数据
	struct SockAndAddr{
		SOCKET sock;
		std::string sIP;
		std::string sPort;
	};
	//每个文件记录 格式 文件名+一组地址
	struct Record{
		std::string sFileName;
		std::vector<std::string> vecAddrs;
	};
	typedef std::vector<Record> Records;
	int searchRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, vector<string>::iterator &iterCols);
	void listFiles(SOCKET sock);
	void requestFile(SOCKET sock, const string &sFileName);
	void addRecord(const string &sFileName, const string &sAddr, Records::iterator &iterRows, const int &iSearchRes);
	void deleteRecord(Records::iterator &iterRows, vector<string>::iterator &iterCols);
	void modifyRecord(SOCKET sock, const string &sFileName, const string &sAddr, const string &opt);
	static void tcpThreadTask(Server *pServer);
public:
	Server(const char *service);
	void run();
private:
	//全局变量
	//存储记录
	Records g_Records;
	//表示 线程数的信号量
	HANDLE g_hThreadNumber;
	HANDLE g_hSockAndAddr;
	//读写锁
	SRWLOCK g_srwLock;
	//刚接收的请求信息
	SockAndAddr sockAndAddr;
	//服务器端口号
	SOCKET tcpSock;
	
};
#endif