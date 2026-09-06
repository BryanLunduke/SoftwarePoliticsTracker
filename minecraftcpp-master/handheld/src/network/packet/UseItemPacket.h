#ifndef NET_MINECRAFT_NETWORK_PACKET__UseItemPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__UseItemPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

#include "../../world/item/ItemInstance.h"

class UseItemPacket: public Packet
{
public:
    int x, y, z, face;
	float clickX, clickY, clickZ;
	int entityId;
    short		  itemId;
	unsigned char itemData;
    ItemInstance item;

    UseItemPacket() {}

    UseItemPacket(int x, int y, int z, int face, const ItemInstance* item, int entityId, float clickX, float clickY, float clickZ)
    :	x(x),
        y(y),
        z(z),
        face(face),
        itemId(item? item->id : 0),
		itemData(item? item->getAuxValue() : 0),
		entityId(entityId),
		clickX(clickX),
		clickY(clickY),
		clickZ(clickZ)
	{}

    UseItemPacket(const ItemInstance* item, int entityId, const Vec3& aim)
    :   face(255),
        itemId(item? item->id : 0),
        itemData(item? item->getAuxValue() : 0),
        entityId(entityId),
        x((int)(aim.x * 32768.0f)),
        y((int)(aim.y * 32768.0f)),
        z((int)(aim.z * 32768.0f))
    {
    }

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_USEITEM));

		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(face);
		bitStream->Write(itemId);
		bitStream->Write(itemData);
		bitStream->Write(entityId);
		bitStream->Write(clickX);
		bitStream->Write(clickY);
		bitStream->Write(clickZ);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(face);
		bitStream->Read(itemId);
		bitStream->Read(itemData);
		bitStream->Read(entityId);
		bitStream->Read(clickX);
		bitStream->Read(clickY);
		bitStream->Read(clickZ);
		item.id = itemId;
		item.setAuxValue(itemData);
		item.count = (itemId == 0 && itemData == 0)? 0 : 1;

	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (UseItemPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__UseItemPacket_H__*/
