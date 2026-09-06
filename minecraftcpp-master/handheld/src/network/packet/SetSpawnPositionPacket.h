#ifndef NET_MINECRAFT_NETWORK_PACKET__SetSpawnPositionPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetSpawnPositionPacket_H__

#include "../Packet.h"

class SetSpawnPositionPacket : public Packet
{
public:
	int entityId;
	int x, z;
	unsigned char y;

	SetSpawnPositionPacket()
	{
	}

	SetSpawnPositionPacket(int x, int y, int z)
	:	x(x),
		y((unsigned char)(y & 0xff)),
		z(z)
	{
	}
	SetSpawnPositionPacket(const Pos& pos)
	:	x(pos.x),
		y((unsigned char)(pos.y & 0xff)),
		z(pos.z)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETSPAWNPOSITION));

		bitStream->Write(x);
		bitStream->Write(z);
		bitStream->Write(y);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(y);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SetSpawnPositionPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SetSpawnPositionPacket_H__*/
