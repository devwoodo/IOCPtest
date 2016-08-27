#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>

#include <iostream>
#include <string>
#include <future>
#include <sstream>

using std::cin;
using std::cout;
using std::endl;

#define BUF_SIZE 1024
void ErrorHandling(char * message);
void sendRoutine(SOCKET sock, char * buf);
void recvRoutine(SOCKET sock, char * buf);

int main(int argc, char * argv[])
{
	// variables declaration
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;
	char message[BUF_SIZE];

	if (argc != 3)
	{
		cout << "Usage: " << argv[0] << " <IP> <port>" << endl;
		exit(1);
	}

	// set WSADATA 
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	// get socket handle
	hSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (hSocket == INVALID_SOCKET)
		ErrorHandling("socket() error");

	// setting SOCKADDR_IN
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(argv[1]);
	servAdr.sin_port = htons(atoi(argv[2]));

	// try connect to server
	if (connect(hSocket, (SOCKADDR *)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");
	else
		cout << "Connected.........." << endl;

	// chatting & echoing routine start
	std::future<void> recvThread(std::async(std::launch::async, recvRoutine, hSocket, message));
	std::future<void> sendThread(std::async(std::launch::async, sendRoutine, hSocket, message));

	sendThread.get();
	recvThread.get();

	//// close socket	//rev
	closesocket(hSocket);
	// wrap up WSADATA
	WSACleanup();

	return 0;
}

void sendRoutine(SOCKET sock, char * buf)
{
	while (1) {
		int strLen = 0;
		std::string msg;
		std::stringstream strbuf;

		cout << "Input message(Q to quit): ";
		std::getline(cin, msg);

		if (msg == "q" || msg == "Q")
			break;

		int pkLen, msgLen;	// packet 전체 크기, msg만의 길이
		int headerLen;

		msgLen = msg.size();
		headerLen = sizeof(size_t);
		pkLen = headerLen + msgLen;
		
		strbuf.write((const char *)(&msgLen), headerLen);
		strbuf << msg;
		//cout << "Processed message: " << strbuf.str() << endl;
		/*send(sock, strbuf.str().c_str(), pkLen, 0);*/ 

		// Test: send by 1 byte.
		auto buf = new char[pkLen];
		int fragSz = 1;
		memcpy_s(buf, pkLen, strbuf.str().c_str(), pkLen);
		cout << "Processed message: ";
		
		for (int i = 0; i < pkLen; i += fragSz) {
			cout << i << ' ';
			send(sock, buf + i, fragSz, 0);
		}
		cout << endl;
	}
	cout << "closing socket.." << endl;
	closesocket(sock);	//rev 
	cout << "socket closed" << endl;
	cout << "sendThread released" << endl;
}

void recvRoutine(SOCKET sock, char * buf)
{
	while (1)
	{
		int recvLen = 0;
		char message[BUF_SIZE];

		recvLen = recv(sock, message, BUF_SIZE - 1, 0);
		if (recvLen == -1) {
			cout << "recvLen is -1. " << endl;
			break;
		}

		message[recvLen] = NULL;

		//rev
		/*while (1)
		{
		readLen += recv(sock, &message[readLen], BUF_SIZE - 1, 0);
		if (readLen >= strLen)
		break;

		if (recvLen == 0)	//rev
		return;
		}
		message[strlen] = 0;
		*/
		cout << "\n\tEcho: " << message << endl;
		cout << "Input message(Q to quit): ";
	}
	cout << "recvThread released" << endl;
}

void ErrorHandling(char * message)
{
	cout << message << endl;
	exit(1);
}