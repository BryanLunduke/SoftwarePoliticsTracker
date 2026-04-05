#ifndef NET_MINECRAFT_NETWORK_PACKET__DropItemPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__DropItemPacket_H__

#include "../Packet.h"

class DropItemPacket: public Packet
{
public:
	DropItemPacket()
	{
	}

	DropItemPacket(int entityId, const ItemInstance& item)
	:  entityId(entityId),
	   item(item),
	   dropType(0)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_DROPITEM));
		bitStream->Write(entityId);
		bitStream->Write(dropType);
		PacketUtil::writeItemInstance(item, bitStream);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(dropType);
		item = PacketUtil::readItemInstance(bitStream);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (DropItemPacket*)this);
	}

    int entityId;
	unsigned char dropType;
    ItemInstance item;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__DropItemPacket_H__*/
