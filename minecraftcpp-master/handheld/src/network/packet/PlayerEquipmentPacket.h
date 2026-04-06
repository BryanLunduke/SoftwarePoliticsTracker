#ifndef NET_MINECRAFT_NETWORK_PACKET__PlayerEquipmentPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlayerEquipmentPacket_H__

#include "../Packet.h"

class PlayerEquipmentPacket : public Packet
{
public:
	int entityId;
	unsigned short itemId;
	unsigned short itemAuxValue;

	PlayerEquipmentPacket()
	{
	}

	PlayerEquipmentPacket(int entityId, int itemId, int data)
	:	entityId(entityId),
		itemId(itemId),
		itemAuxValue(data)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLAYEREQUIPMENT));

		bitStream->Write(entityId);
		bitStream->Write(itemId);
		bitStream->Write(itemAuxValue);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(itemId);
		bitStream->Read(itemAuxValue);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (PlayerEquipmentPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__PlayerEquipmentPacket_H__*/
