#include "p2pClient.h"
#include "p2pServer.h"
#include <iostream>
#include <string>
#define SERVER_IP "127.0.0.1"
#define MASTER_SERVER_PORT "7777"
using namespace std;
#pragma comment(lib,"ws2_32.lib")
int main()
{
	cout << "Server or Client (s/c) ?  >> ";
	string str;
	cin >> str;
	if (str == "s" || str=="S")
	{
		Server myServer(MASTER_SERVER_PORT);
		myServer.run();
	}
	else if (str == "c" || str=="C")
	{
		cout << "input local port >> ";
		cin >> str;
		cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n" << endl;
		cout << "SUPPORT COMMAND :" << endl;
		cout << "\tADD <FILENAME>\n\tDELETE <FILENAME>\n\tREQUEST <FILENAME>\n\tLIST\n\tQUIT" << endl;
		cout << "\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n" << endl;
		Client myClient(SERVER_IP, MASTER_SERVER_PORT, str.c_str());
		myClient.run();
	}
	system("pause");
    return 0;
}
