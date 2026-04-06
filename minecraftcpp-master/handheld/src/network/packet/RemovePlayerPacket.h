#ifndef NET_MINECRAFT_NETWORK_PACKET__RemovePlayerPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__RemovePlayerPacket_H__

#include "../Packet.h"
#include "../../world/entity/player/Player.h"

class RemovePlayerPacket : public Packet
{
public:
	RemovePlayerPacket() {}

	RemovePlayerPacket(const Player* p)
	:  entityId(p->entityId),
	   owner(p->owner)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_REMOVEPLAYER));

		bitStream->Write(entityId);
		bitStream->Write(owner);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(owner);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (RemovePlayerPacket*)this);
	}

	int entityId;
	RakNet::RakNetGUID owner;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__RemovePlayerPacket_H__*/
