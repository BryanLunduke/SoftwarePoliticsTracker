#ifndef NET_MINECRAFT_NETWORK_PACKET__RemoveEntityPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__RemoveEntityPacket_H__

#include "../Packet.h"

class RemoveEntityPacket : public Packet
{
public:

	int entityId;

	RemoveEntityPacket()
	{
	}

	RemoveEntityPacket(int entityId)
	:	entityId(entityId)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_REMOVEENTITY));

		bitStream->Write(entityId);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (RemoveEntityPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__RemoveEntityPacket_H__*/
