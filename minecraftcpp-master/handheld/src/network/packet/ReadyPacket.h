#ifndef NET_MINECRAFT_NETWORK_PACKET__ReadyPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ReadyPacket_H__

#include "../Packet.h"

class ReadyPacket: public Packet
{
public:
    static const char READY_UNDEFINED = 0;
	static const char READY_CLIENTGENERATION = 1;
	static const char READY_REQUESTEDCHUNKS = 2;

	ReadyPacket()
	:   type(READY_UNDEFINED)
	{
	}

	ReadyPacket(char type)
    :   type(type)
    {
    }

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_READY));
		bitStream->Write(type);
	}

	void read(RakNet::BitStream* bitStream)
	{
        bitStream->Read(type);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (ReadyPacket*)this);
	}

    char type;
};

#endif /*#NET_MINECRAFT_NETWORK_PACKET__ReadyPacket_H__*/
