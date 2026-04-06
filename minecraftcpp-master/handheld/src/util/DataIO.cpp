#include "DataIO.h"

//
// BytesDataOutput
//

void BytesDataOutput::writeString( const std::string& v )
{
    int length = v.length() & 0x7fff;
	writeShort(length);
	writeBytes(v.c_str(), length);
	//LOGI("Writing: %d bytes as String: %s\n", v.length(), v.c_str());
}

//
// BytesDataInput
//
std::string BytesDataInput::readString() {
	int len = readShort();
	if (len > MAX_STRING_LENGTH - 1)
		len = MAX_STRING_LENGTH - 1;
	char* buffer = new char[len + 1];
	readBytes(buffer, len);
	buffer[len] = 0;

	std::string out(buffer);
	delete[] buffer;
	return out;
}
