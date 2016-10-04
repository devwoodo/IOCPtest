#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <winsock2.h>

#include <iostream>
#include <string>
#include <future>
#include <sstream>

#include "../../chatters/chatters/header_server.h"

using std::cin;
using std::cout;
using std::endl;

void sending(SOCKET sock, std::shared_ptr<Packet_Base> shPk);
void receiving(SOCKET sock);

int main(int argc, char * argv[])
{
	// variables declaration
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN servAdr;

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

	std::future<void> recvThread(std::async(std::launch::async, receiving, hSocket));
	
	////////////////////////////////////////////////////////////////////////////////////////////
	// Test code.
	UserInfoToken utk;

	// PK_CS_LOGIN_REQUEST
	{
		// 
		cout << "Sending PK_CS_LOGIN_REQUEST. ";
		cout << "Press any key to continue. ";
		char ch;
		cin >> ch;

		// create packet
		auto pk = std::make_shared<PK_CS_LOGIN_REQUEST>();

		// fill packet
		pk->userId = "a";
		pk->userPassword = "1234";

		// sending packet
		cout << "Call sending(..)" << endl;
		sending(hSocket, pk);

		//// receiving packet
		//cout << "Waiting receiving.." << endl;
		//auto shRecvPk = receiving(hSocket);

		//// print packet content
		//cout << shRecvPk->get_buf().str() << endl;
		//
		//// special process
		//// check packet type
		//if (shRecvPk->id != PTYPE::PT_SC_LOGIN_ACCEPT)
		//{
		//	cout << "Login failed." << endl;
		//	exit(1);
		//}
		//// casting packet type
		//auto shPk = std::dynamic_pointer_cast<PK_SC_LOGIN_ACCEPT>(shRecvPk);
		//
		//utk = shPk->userTk;
	}

	// PK_CS_LOBBY_LOAD_ROOMLIST
	{
		//
		cout << "Sending PK_CS_LOBBY_LOAD_ROOMLIST. ";
		cout << "Press any key to continue. ";
		char ch;
		cin >> ch;

		// create packet
		cout << "Create packet." << endl;
		auto pk = std::make_shared<PK_CS_LOBBY_LOAD_ROOMLIST>();

		// fill packet

		// sending packet
		cout << "Call sending(..)" << endl;
		sending(hSocket, pk);

		//// receiving packet
		//auto shRecvPk = receiving(hSocket);

		//// print packet content
		//cout << shRecvPk->get_buf().str() << endl;

		//// special process

		// 검토 사항	//rev
		// 1. 리스트가 없는 경우?
	}



	/*
	// PK_*
	{
	//
	cout << "Sending PK_*. ";
	cout << "Press any key to continue. ";
	char ch;
	cin >> ch;

	// create packet
	auto pk = std::make_shared<PK_*>();

	// fill packet

	// sending packet
	cout << "Call sending(..)" << endl;
	sending(hSocket, pk);
	std::future<void> sendThread(std::async(std::launch::async, sending, hSocket, pk));	//rev

	// receiving packet
	auto shRecvPk = receiving(hSocket);

	// print packet content
	cout << shRecvPk->get_buf().str() << endl;

	// special process

	// 검토 사항

	}
	*/
	
	/*
	// chatting & echoing routine start
	std::future<void> recvThread(std::async(std::launch::async, recvRoutine, hSocket, message));
	std::future<void> sendThread(std::async(std::launch::async, sendRoutine, hSocket, message));

	sendThread.get();
	recvThread.get();
	*/

	recvThread.get();

	//// close socket
	closesocket(hSocket);

	// wrap up WSADATA
	WSACleanup();

	cout << "Closeing socket successfully." << endl;

	return 0;
}

void sending(SOCKET sock, std::shared_ptr<Packet_Base> shPk)
{
	char * buf;
	size_t pkSz;

	shPk->serialize();	
	
	pkSz = shPk->get_packetSize();
	cout << "Packet size: " << pkSz << endl;
	
	buf = new char[pkSz];
	memset(buf, 0, pkSz);
	memcpy_s(buf, pkSz, shPk->get_buf().str().c_str(), pkSz);

	cout << "Sending packet.." << endl;
	send(sock, buf, pkSz, 0);

	delete[] buf;
}
void receiving(SOCKET sock)
{
	while (1) {
		cout << "Waiting receiving.." << endl;

		int transmitted, toReceive;
		size_t bufLen;
		char * buf;

		// receive packet size(Packet's internal buffer length)
		toReceive = sizeof(size_t);
		while (1)
		{
			transmitted = recv(sock, (char *)&bufLen, toReceive, 0);
			toReceive -= transmitted;
			if (toReceive <= 0 || transmitted == 0)
				break;
		}
		if (transmitted == 0) {
			cout << "Connection closed." << endl;
			break;
		}

		// receive packet(internal buffer)
		toReceive = bufLen;
		buf = new char[bufLen];
		while (1)
		{
			transmitted = recv(sock, buf, toReceive, 0);
			toReceive -= transmitted;
			if (toReceive <= 0 || transmitted == 0)
				break;
		}
		if (transmitted == 0) {
			cout << "Connection closed." << endl;
			break;
		}

		auto shPk = extractSCPacket(buf, bufLen);

		cout << shPk->get_buf().str() << endl;
	}

	return;
}
//
//void receiving(SOCKET sock)
//{
//	int transmitted, toReceive;
//	size_t bufLen;
//	char * buf;
//
//	// receive packet size(Packet's internal buffer length)
//	toReceive = sizeof(size_t);
//	while (1)
//	{
//		transmitted = recv(sock, (char *)&bufLen, toReceive, 0);
//		toReceive -= transmitted;
//		if (toReceive <= 0 || transmitted == 0)
//			break;
//	}
//	if (transmitted == 0) {
//		cout << "Connection closed." << endl;
//	}
//
//	// receive packet(internal buffer)
//	toReceive = bufLen;
//	buf = new char[bufLen];
//	while (1)
//	{
//		transmitted = recv(sock, buf, toReceive, 0);
//		toReceive -= transmitted;
//		if (toReceive <= 0 || transmitted == 0)
//			break;
//	}
//	if (transmitted == 0) {
//		cout << "Connection closed." << endl;
//	}
//
//	auto shPk = extractSCPacket(buf, bufLen);
//
//	cout << shPk->get_buf().str() << endl;
//
//	return;
//}
//
