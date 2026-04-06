#ifndef NET_MINECRAFT_NETWORK_PACKET__RespawnPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__RespawnPacket_H__

#include "../Packet.h"

class RespawnPacket: public Packet
{
public:
	RespawnPacket()
	{
	}

	RespawnPacket(const Player* p)
	:	x(p->x),y(p->y),z(p->z),
		entityId(p->entityId)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_RESPAWN));
		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (RespawnPacket*)this);
	}

	float x;
	float y;
	float z;
	int entityId;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__RespawnPacket_H__*/
