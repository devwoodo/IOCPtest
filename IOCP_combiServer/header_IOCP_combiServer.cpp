#include "header_IOCP_combiServer.h"



PerIoData::PerIoData() : _buffer(nullptr), _bufferLen(0), _refCount(0)
{
	wsaBuf.len = _bufferLen;
	wsaBuf.buf = _buffer;
}
PerIoData::PerIoData(size_t bufSz) : PerIoData()
{
	allocBuffer(bufSz);
}
PerIoData::~PerIoData()
{
	_releaseBuffer();
	cout << "~PER_IO_DATA()" << endl;
}
int PerIoData::get_refCount() const
{ 
	return _refCount;
}
void PerIoData::inc_refCount()
{
	_refCount++;
}
void PerIoData::allocBuffer(size_t bufSz)
{
	_releaseBuffer();
	_buffer = new char[bufSz];
	_bufferLen = bufSz;
	wsaBuf.len = bufSz;
	wsaBuf.buf = _buffer;
}
void PerIoData::set_Buffer(char * bufPtr, int bufSz)
{
	if(_buffer != nullptr)
		delete[] _buffer;

	_buffer = bufPtr;
	_bufferLen = bufSz;
	wsaBuf.len = bufSz;
	wsaBuf.buf = _buffer;
}
char * PerIoData::get_buffer() const
{
	return _buffer;
}
size_t PerIoData::get_bufferLen() const
{
	return _bufferLen;
}
void PerIoData::set_refCount(int newVal)
{
	_refCount = newVal;
}
void PerIoData::dec_refCount()
{
	_refCount--;
}
void PerIoData::_releaseBuffer()
{
	if (_buffer != nullptr)
		delete[] _buffer;

	_bufferLen = 0;
	wsaBuf.len = 0;
	wsaBuf.buf = nullptr;
}
