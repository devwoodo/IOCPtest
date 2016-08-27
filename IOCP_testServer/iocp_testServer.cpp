/*************************************************************************
 * wsarecv(..)를 통해 io completion notice만 받고 실제 message는 recv(..)를
  통해 받는 방식.
	- wsarecv(..)에 건네주는 WSABUF의 크기를 0으로.
 *************************************************************************/


#include <iostream>
//#include <stdio.h>
#include <cstdio>
//#include <stdlib.h>
#include <cstdlib>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

#define BUF_SIZE 7
#define BUF_ZERO_SIZE 0
#define READ 3
#define WRITE 5

using std::cin;
using std::cout;
using std::endl;

typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

// do not allocate object of PerIoData type using malloc(). Use only new op.
typedef struct PerIoData
{
public:
	PerIoData() : _refCount(1)
	{}

	void set_refCount(int newVal) { _refCount = newVal; }
	int get_refCount() { return _refCount; }

	void operator delete(void * p)
	{
		auto targetPtr = static_cast<LPPER_IO_DATA>(p);
		if (targetPtr->_refCount <= 1)
		{
			cout << "~PER_IO_DATA(addr: " << p << ") called. Object deleted." << endl;
			free(p);
			return;
		}
		targetPtr->_refCount--;
		cout << "~PER_IO_DATA(addr: " << p << ") called. refCount: " << targetPtr->_refCount << endl;
	}
public:
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;		// read mode / write mode distinguisher
private:
	int _refCount;
} PER_IO_DATA, *LPPER_IO_DATA;

DWORD WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char * mesaage);

int main(int argc, char * argv[])
{
	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags = 0;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error");

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo);
	for (i = 0; i < (int)(sysInfo.dwNumberOfProcessors); i++)
		_beginthreadex(NULL, 0, (unsigned int(__stdcall *)(void *))EchoThreadMain, (LPVOID)hComPort, 0, NULL);

	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	bind(hServSock, (SOCKADDR *)&servAdr, sizeof(servAdr));
	listen(hServSock, 5);

	cout << "server start successfully.." << endl;

	while (1)
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);

		hClntSock = accept(hServSock, (SOCKADDR *)&clntAdr, &addrLen);
		handleInfo = new(PER_HANDLE_DATA);
		//handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		ioInfo = new PER_IO_DATA();
		//ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_ZERO_SIZE;		//rev
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, (LPDWORD)&recvBytes, (LPDWORD)&flags, &(ioInfo->overlapped), NULL);
	}
	return 0;
}

DWORD WINAPI EchoThreadMain(LPVOID pComPort)
{
	HANDLE hComPort = (HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	while (1)
	{
		GetQueuedCompletionStatus(hComPort, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED *)&ioInfo, INFINITE);
		cout << "GetQueuedComplitionStatus bytesTrans = " << bytesTrans << endl;
		sock = handleInfo->hClntSock;

		if (ioInfo->rwMode == READ)
		{
			cout << "message received. ";

			//rev
			char recvBuffer[BUF_SIZE];
			bytesTrans = recv(sock, recvBuffer, BUF_SIZE - 1, 0);	//rev //? error원인?	>> 이미 닫힌 socket에 대해 recv call..
			recvBuffer[bytesTrans] = 0;
			cout << "bytesTrans = " << bytesTrans << endl;
			if (bytesTrans == 0)	// EOF 전송시
			{
				std::cout << "closing socket(bytesTrans == 0)" << std::endl;
				closesocket(sock);
				free(handleInfo); delete(ioInfo);
				continue;
			}
			//Sleep(3000);   	// 서버 측 echo에 딜레이를 줘서 서버 tcp 
								//buffer에 데이터를 input하고, buffer에 data가
								//남아있지만 client로부터 추가 전송은 없는 
								//상태에서 WSARecv(..) call 동작을 살펴보기
								//위함

			std::cout << "\tmsg: " << recvBuffer << std::endl;

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			strncpy_s(ioInfo->buffer, recvBuffer, BUF_SIZE);
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			/*
			if (bytesTrans == 0)	// EOF 전송시
			{
			std::cout << "bytesTrans == 0" << std::endl;
			closesocket(sock);
			free(handleInfo); free(ioInfo);
			continue;
			}

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			*/

			ioInfo = new PER_IO_DATA();
			//ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_ZERO_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			puts("message sent");
			delete(ioInfo);
		}
	}
	return 0;
}

void ErrorHandling(char * message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}


//? client에서 연결 close 할 때 server에서 오류 나는데..?
//	>> client에서 closesocket() 시 서버에 측에선 이 신호를 받아 wsarecv가 리턴값(bytetrans)으로 
//		0을 반환하고 socket close. 그런데 현재 implement 상으론 wsarecv가 무조건 0을 반환하게 
//		되있기 때문에 (ioData->wsaData->len = 0으로 정의) socket close 신호를 구분할 수가 없음.
//		 따라서 socket이 닫힌 줄 모르고 wsarecv() 뒤에 닫힌 socket을 대상으로 recv()를 부르기
//		때문에 오류 발생.