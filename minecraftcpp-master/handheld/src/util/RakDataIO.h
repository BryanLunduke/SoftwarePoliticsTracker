#ifndef RAKDATAIO_H__
#define RAKDATAIO_H__

#include <cstring>
#include <string>

#include "DataIO.h"

// Uses BitStream as a growing buffer
class RakDataOutput: public BytesDataOutput {
public:
	//RakDataOutput() {}
	RakDataOutput(RakNet::BitStream& bitstream)
	:	_bitStream(bitstream)
	{}

	virtual void writeBytes(const void* data, int bytes) {
		_bitStream.WriteBits((const unsigned char*)data, bytes * 8);
	}

	RakNet::BitStream& getBitStream() {
		return _bitStream;
	}

private:
	RakNet::BitStream& _bitStream;
};


class RakDataInput: public BytesDataInput {
public:
	//RakDataOutput() {}
	RakDataInput(RakNet::BitStream& bitstream)
		:	_bitStream(bitstream)
	{}

	virtual void readBytes(void* data, int bytes) {
		_bitStream.ReadBits((unsigned char*) data, bytes * 8);
	}

	RakNet::BitStream& getBitStream() {
		return _bitStream;
	}

private:
	RakNet::BitStream& _bitStream;
};

#endif /*RAKDATAIO_H__*/
