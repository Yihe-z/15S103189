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
	//�����߳�ʱ���ݵ�����
	struct SockAndAddr{
		SOCKET sock;
		std::string sIP;
		std::string sPort;
	};
	//ÿ���ļ���¼ ��ʽ �ļ���+һ���ַ
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
	//ȫ�ֱ���
	//�洢��¼
	Records g_Records;
	//��ʾ �߳������ź���
	HANDLE g_hThreadNumber;
	HANDLE g_hSockAndAddr;
	//��д��
	SRWLOCK g_srwLock;
	//�ս��յ�������Ϣ
	SockAndAddr sockAndAddr;
	//�������˿ں�
	SOCKET tcpSock;
	
};
#endif