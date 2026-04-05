#ifndef NET_MINECRAFT_NETWORK_PACKET__PlaceBlockPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlaceBlockPacket_H__

#include "../Packet.h"

class PlaceBlockPacket : public Packet
{
public:

	// the id of the player who is placing the block, used to animate the player
	int entityId;
	int x, z;
	unsigned char y, facing, blockId, blockData;

	PlaceBlockPacket()
	{
	}

	PlaceBlockPacket(int entityId, int x, int y, int z, int facing, int blockId, int blockData)
	:	entityId(entityId),
		x(x),
		y((unsigned char)(y & 0xff)),
		z(z),
		facing ((unsigned char)(facing & 0xff)),
		blockId((unsigned char)(blockId & 0xff)),
		blockData((unsigned char)(blockData & 0xff))
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLACEBLOCK));

		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(z);
		bitStream->Write(y);
		bitStream->Write(facing);
		bitStream->Write(blockId);
		bitStream->Write(blockData);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(z);
		bitStream->Read(y);
		bitStream->Read(facing);
		bitStream->Read(blockId);
		bitStream->Read(blockData);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (PlaceBlockPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__PlaceBlockPacket_H__*/
