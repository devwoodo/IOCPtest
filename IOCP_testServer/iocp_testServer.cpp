/*************************************************************************
 * wsarecv(..)�� ���� io completion notice�� �ް� ���� message�� recv(..)��
  ���� �޴� ���.
	- wsarecv(..)�� �ǳ��ִ� WSABUF�� ũ�⸦ 0����.
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
			bytesTrans = recv(sock, recvBuffer, BUF_SIZE - 1, 0);	//rev //? error����?	>> �̹� ���� socket�� ���� recv call..
			recvBuffer[bytesTrans] = 0;
			cout << "bytesTrans = " << bytesTrans << endl;
			if (bytesTrans == 0)	// EOF ���۽�
			{
				std::cout << "closing socket(bytesTrans == 0)" << std::endl;
				closesocket(sock);
				free(handleInfo); delete(ioInfo);
				continue;
			}
			//Sleep(3000);   	// ���� �� echo�� �����̸� �༭ ���� tcp 
								//buffer�� �����͸� input�ϰ�, buffer�� data��
								//���������� client�κ��� �߰� ������ ���� 
								//���¿��� WSARecv(..) call ������ ���캸��
								//����

			std::cout << "\tmsg: " << recvBuffer << std::endl;

			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			strncpy_s(ioInfo->buffer, recvBuffer, BUF_SIZE);
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);

			/*
			if (bytesTrans == 0)	// EOF ���۽�
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


//? client���� ���� close �� �� server���� ���� ���µ�..?
//	>> client���� closesocket() �� ������ ������ �� ��ȣ�� �޾� wsarecv�� ���ϰ�(bytetrans)���� 
//		0�� ��ȯ�ϰ� socket close. �׷��� ���� implement ������ wsarecv�� ������ 0�� ��ȯ�ϰ� 
//		���ֱ� ������ (ioData->wsaData->len = 0���� ����) socket close ��ȣ�� ������ ���� ����.
//		 ���� socket�� ���� �� �𸣰� wsarecv() �ڿ� ���� socket�� ������� recv()�� �θ���
//		������ ���� �߻�.