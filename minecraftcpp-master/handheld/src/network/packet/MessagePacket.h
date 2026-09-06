#ifndef NET_MINECRAFT_NETWORK_PACKET__MessagePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__MessagePacket_H__

#include "../Packet.h"

class MessagePacket : public Packet
{
public:

	RakNet::RakString message;

	MessagePacket()
	{
	}

	MessagePacket(const RakNet::RakString& message)
	:	message(message)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_MESSAGE));

		bitStream->Write(message);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(message);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (MessagePacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__MessagePacket_H__*/
