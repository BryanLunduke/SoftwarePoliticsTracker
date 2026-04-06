#ifndef NET_MINECRAFT_NETWORK_PACKET__RequestChunkPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__RequestChunkPacket_H__

#include "../Packet.h"

class RequestChunkPacket : public Packet
{
public:

	int x, z;

	RequestChunkPacket()
	{
	}

	RequestChunkPacket(int _x, int _z)
	{
		x = _x;
		z = _z;
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_REQUESTCHUNK));

		bitStream->Write(x);
		bitStream->Write(z);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(z);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (RequestChunkPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__RequestChunkPacket_H__*/
