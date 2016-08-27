#pragma once

#pragma once

#include <iostream>
//#include <stdio.h>
#include <cstdio>
//#include <stdlib.h>
#include <cstdlib>
#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

#include <memory>

#define BUF_SIZE 128
#define HEADER_SIZE sizeof(size_t)		//rev
#define BUF_ZERO_SIZE 0
#define READ_HEADER 3
#define READ_PACKET 4
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
	PerIoData();
	PerIoData(size_t bufSz);
	~PerIoData();

	int get_refCount() const;
	void inc_refCount();
	void allocBuffer(size_t bufSz);
	void set_Buffer(char * bufPtr, int bufSz);
	char * get_buffer() const;
	size_t get_bufferLen() const;

	void operator delete(void * p)
	{
		auto targetPtr = static_cast<LPPER_IO_DATA>(p);
		if (targetPtr->_refCount <= 1)
		{
			cout << "delete PER_IO_DATA(addr: " << p << ") called. Object deleted." << endl;
			free(p);
			return;
		}
		targetPtr->_refCount--;
		cout << "delete PER_IO_DATA(addr: " << p << ") called. refCount: " << targetPtr->_refCount << endl;
	}
public:
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	int rwMode;		// read mode / write mode distinguisher
private:
	void set_refCount(int newVal);
	void dec_refCount();
	void _releaseBuffer();
private:
	char * _buffer;	//rev
	size_t _bufferLen;
	int _refCount;
} PER_IO_DATA, *LPPER_IO_DATA;