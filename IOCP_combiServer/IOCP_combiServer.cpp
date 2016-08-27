/*************************************************************************
 * wsarecv(..)를 통해 io completion notice를 받고, 앞으로 받을 메시지의 
  크기(헤더부분)를 받아옴. 이후 실제 메시지는 recv(..) 통해 받음
	- WSARecv(..)에 전해주는 WSABUF의 크기를 헤더크기(size_t)로 설정. 
	- 패킷 크기를 알아낸 후 recv를 통해 그 크기만큼 받아옴.
*************************************************************************/

#include "header_IOCP_combiServer.h"

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
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);

		ioInfo = new PER_IO_DATA(HEADER_SIZE);
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		//ioInfo->wsaBuf.len = HEADER_SIZE;
		//ioInfo->wsaBuf.buf = ioInfo->get_buffer();
		ioInfo->rwMode = READ_HEADER;
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
		cout << "GetQueuedComplitionStatus(mode: "<< ioInfo->rwMode << ") bytesTrans = " << bytesTrans << endl;
		sock = handleInfo->hClntSock;
				
		if (ioInfo->rwMode == READ_HEADER)
		{
			cout << "|READ_HEADER| sock: " << sock << endl;
			if (bytesTrans == 0)	// EOF 전송시
			{
				std::cout << "|READ_HEADER| closing socket(bytesTrans == 0) socket: " << sock << std::endl;
				closesocket(sock);
				free(handleInfo); delete(ioInfo);
				continue;
			}

			// 받은 바이트수 체크
			ioInfo->wsaBuf.len -= bytesTrans;
			ioInfo->wsaBuf.buf += bytesTrans;
			if (ioInfo->wsaBuf.len != 0)
			{
				cout << "|READ_HEADER| fragmented packet. WSARecv() called again." << endl;
				WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
			else {
				// ioInfo reset to wait for packet
				size_t pkSize = static_cast<int>(*(ioInfo->get_buffer()));
				cout << "|READ_HEADER| Packet size: " << pkSize << ". rwMode changed to READ_PACKET." << endl;
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				ioInfo->allocBuffer(pkSize);
				ioInfo->rwMode = READ_PACKET;

				// wsarecv call
				WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
		}
		else if (ioInfo->rwMode == READ_PACKET)
		{
			cout << "|READ_PACKET| sock: " << sock << endl;

			if (bytesTrans == 0)
			{
				std::cout << "|READ_PACKET| closing socket(bytesTrans == 0)" << std::endl;
				closesocket(sock);
				free(handleInfo); delete(ioInfo);
				continue;
			}

			// 받은 바이트수 체크
			ioInfo->wsaBuf.len -= bytesTrans;
			ioInfo->wsaBuf.buf += bytesTrans;
			if (ioInfo->wsaBuf.len != 0)
			{
				cout << "|READ_PACKET| fragmented packet. WSARecv() called again." << endl;
				WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
			else {
				// echo message
				size_t msgLen = ioInfo->get_bufferLen();
				cout << "|READ_PACKET| message length: " << msgLen << ". process echo." << endl;
				//cout << "message: " << ioInfo->get_buffer() << endl;

				LPPER_IO_DATA echoIoInfo = new PerIoData(msgLen);
				memset(&(echoIoInfo->overlapped), 0, sizeof(OVERLAPPED));
				//strncpy_s(echoIoInfo->get_buffer(), echoIoInfo->get_bufferLen(), ioInfo->get_buffer(), ioInfo->get_bufferLen());
				memcpy_s(echoIoInfo->get_buffer(), echoIoInfo->get_bufferLen(), ioInfo->get_buffer(), ioInfo->get_bufferLen());
				echoIoInfo->rwMode = WRITE;
				WSASend(sock, &(echoIoInfo->wsaBuf), 1, NULL, 0, &(echoIoInfo->overlapped), NULL);

				// ioInfo reset to wait for header
				cout << "|READ_PACKET| Header size: " << HEADER_SIZE << ". rwMode changed to READ_HEADER." << endl;
				memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
				ioInfo->allocBuffer(HEADER_SIZE);
				ioInfo->rwMode = READ_HEADER;

				// wsarecv call
				WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
			}
		}
		else
		{
			puts("|WRITE| message sent");
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

