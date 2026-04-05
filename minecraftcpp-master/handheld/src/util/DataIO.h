#ifndef DATAIO_H__
#define DATAIO_H__

#include <cstring>
#include <string>
#include <climits>

#include "../platform/log.h"

// Interface for writing primitives to a stream
class IDataOutput {
public:
	virtual ~IDataOutput() {}

	// Write a "Pascal" string [Len(Short)][Characters, len={strlen()-1,s.length()}]
	//virtual void writePStr(const char* v, int len = -1) = 0;
	//virtual void writeCStr(const char* v, int len = -1) = 0;
	virtual void writeString(const std::string& v) = 0;
	virtual void writeFloat(float v)	= 0;
	virtual void writeDouble(double v)	= 0;

	virtual void writeByte(char v)		= 0;
	virtual void writeShort(short v)	= 0;
	virtual void writeInt(int v)		= 0;
	virtual void writeLongLong(long long v) = 0;

	virtual void writeBytes(const void* data, int bytes) = 0;
};


// Interface for reading primitives from a stream
class IDataInput {
public:
	virtual ~IDataInput() {}

	virtual std::string readString() = 0;
	//virtual void		readPStr(char**) = 0;
	//virtual void		readCStr(char** s, int len = -1) = 0;
	virtual float		readFloat()		= 0;
	virtual double		readDouble()	= 0;

	virtual char		readByte()		= 0;
	virtual short		readShort()		= 0;
	virtual int			readInt()		= 0;
	virtual long long	readLongLong()	= 0;

	virtual void		readBytes(void* data, int bytes) = 0;
};

/**
 * Redirects all calls to writeBytes
 */
class BytesDataOutput: public IDataOutput {
public:
	//virtual void writePStr(const char* v, int len = -1);
	//virtual void writeCStr(const char* v, int len = -1);
	virtual void writeString(const std::string& v);
	virtual void writeFloat(float v) {
		writeBytes(&v, 4);
	}
	virtual void writeDouble(double v) {
		writeBytes(&v, 8);
	}
	virtual void writeByte(char v) {
		writeBytes(&v, 1);
	}
	virtual void writeShort(short v) {
		writeBytes(&v, 2);
	}
	virtual void writeInt(int v) {
		writeBytes(&v, 4);
	}
	virtual void writeLongLong(long long v) {
		writeBytes(&v, 8);
	}
	virtual void writeBytes(const void* data, int bytes) = 0;
};

/**
 * Redirects all calls to readBytes
 */
class BytesDataInput: public IDataInput {
public:
	//virtual void readPStr(char** s);
	//virtual void readCStr(char* s, int len = -1);
	virtual std::string readString();
	virtual float readFloat()	{
		float o;
		readBytes(&o, 4);
		return o;
	}
	virtual double readDouble() {
		double o;
		readBytes(&o, 8);
		return o;
	}
	virtual char readByte()	{
		char o;
		readBytes(&o, 1);
		return o;
	}
	virtual short readShort()	{
		short o;
		readBytes(&o, 2);
		return o;
	}
	virtual int readInt()	{
		int o;
		readBytes(&o, 4);
		return o;
	}
	virtual long long readLongLong() {
		long long o;
		readBytes(&o, 8);
		return o;
	}
	virtual void readBytes(void* data, int bytes) = 0;

private:
	static const int MAX_STRING_LENGTH = SHRT_MAX;
	static char _charBuffer[MAX_STRING_LENGTH];
};

//class MemoryDataInput: public BytesDataInput {
//public:
//	MemoryDataInput(const char* data, int size)
//	:	_buffer(data),
//		_size(size),
//		_index(0)
//	{}
//	MemoryDataInput(const unsigned char* data, int size)
//	:	_buffer((const char*)data),
//		_size(size),
//		_index(0)
//	{}
//
//	void readBytes(void* data, int bytes) {
//		if (bytes <= 0) {
//			if (bytes < 0) LOGE("Error: %d bytes NOT read @ MemoryDataInput::readBytes!\n", bytes);
//			return;
//		}
//		int left = _size - _index;
//		if (bytes > left) {
//			LOGE("ERROR: Not enough bytes left in buffer @ MemoryDataInput::readBytes (%d/%d).\n", bytes, left);
//			_index = _size;
//			return;
//		}
//		memcpy((char*)data, &_buffer[_index], bytes);
//		_index += bytes;
//	}
//
//private:
//	int _index;
//	int _size;
//	const char* _buffer;
//};
//
//
//class MemoryDataOutput: public BytesDataOutput {
//public:
//	MemoryDataOutput(char* data, int size)
//	:	_buffer(data),
//		_size(size),
//		_index(0)
//	{}
//	MemoryDataOutput(unsigned char* data, int size)
//	:	_buffer((char*)data),
//		_size(size),
//		_index(0)
//	{}
//
//	void writeBytes(const void* data, int bytes) {
//		if (bytes <= 0) {
//			LOGW("Warning: %d bytes read @ MemoryDataOutput::writeBytes!\n", bytes);
//			return;
//		}
//		int left = _size - _index;
//		if (bytes > left) {
//			LOGE("ERROR: Not enough bytes left in buffer @ MemoryDataOutput::writeBytes (%d/%d).\n", bytes, left);
//			_index = _size;
//			return;
//		}
//		memcpy(&_buffer[_index], (const char*)data, bytes);
//		_index += bytes;
//	}
//
//private:
//	int _index;
//	int _size;
//	char* _buffer;
//};


class PrintStream {
public:
	void print(const std::string& s) {
	}
	void println(const std::string& s) {
		print(s); print("\n");
	}
};

class FileError {
public:
	static const int NOT_OPENED = 1;
	static const int NOT_FULLY_HANDLED = 2;
};
//
//class FileIO {
//public:
//	FileIO(const std::string& filePath, const char* mode)
//	:	_filePath(filePath),
//		_opened(false),
//		_errCode(0),
//		_fp(0),
//		_doClose(true)
//	{
//		if (mode)
//			openWithMode(mode);
//	}
//	FileIO(FILE* fp)
//	:	_filePath("<fp>"),
//		_opened(fp != NULL),
//		_errCode(0),
//		_fp(fp),
//		_doClose(false)
//	{}
//
//	~FileIO() {
//		if (_doClose)
//			close();
//	}
//
//	bool isValid() {
//		return _opened;
//	}
//
//	int getError() {
//		int err = _errCode;
//		_errCode = 0;
//		return err;
//	}
//
//	bool close() {
//		if (!_fp)
//			return false;
//
//		fclose(_fp);
//		_fp = NULL;
//		_opened = false;
//		return true;
//	}
//
//protected:
//	void openWithMode(const char* mode) {
//		if (_fp) return;
//		_fp = fopen(_filePath.c_str(), mode);
//		_opened = (_fp != NULL);
//	}
//
//	std::string _filePath;
//	int  _errCode;
//	bool _opened;
//	bool _doClose;
//
//	FILE* _fp;
//};
//
//class FileDataOutput: public BytesDataOutput, public FileIO
//{
//public:
//	FileDataOutput(const std::string& filePath)
//	:	FileIO(filePath, "wb"),
//		bytesWritten(0)
//	{
//	}
//	FileDataOutput(FILE* fp)
//	:	FileIO(fp),
//		bytesWritten(0)
//	{}
//
//	virtual void writeBytes(const void* data, int bytes) {
//		if (!_fp) {
//			_errCode |= FileError::NOT_OPENED;
//			return;
//		}
//
//		int bytesWritten = fwrite(data, 1, bytes, _fp);
//		if (bytesWritten != bytes)
//			_errCode |= FileError::NOT_FULLY_HANDLED;
//
//		this->bytesWritten += bytesWritten;
//	}
//
//	int bytesWritten;
//};
//
//class FileDataInput: public BytesDataInput, public FileIO
//{
//public:
//	FileDataInput(const std::string& filePath)
//	:	FileIO(filePath, "rb")
//	{
//	}
//	FileDataInput(FILE* fp)
//	:	FileIO(fp)
//	{}
//
//	virtual void readBytes(void* data, int bytes) {
//		if (!_fp) {
//			_errCode |= FileError::NOT_OPENED;
//			return;
//		}
//
//		if (fread(data, 1, bytes, _fp) != bytes)
//			_errCode |= FileError::NOT_FULLY_HANDLED;
//	}
//};

#endif /*DATAIO_H__*/
