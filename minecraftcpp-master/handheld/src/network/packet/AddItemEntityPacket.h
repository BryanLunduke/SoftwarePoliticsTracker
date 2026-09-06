#ifndef NET_MINECRAFT_NETWORK_PACKET__AddItemEntityPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AddItemEntityPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"
#include "../../world/entity/item/ItemEntity.h"
#include "../../world/item/ItemInstance.h"
#include "../../util/Mth.h"

class AddItemEntityPacket: public Packet {
public:
    AddItemEntityPacket() {
    }

    AddItemEntityPacket(const ItemEntity* itemEntity)
    :	id(itemEntity->entityId),
        itemId(itemEntity->item.id),
        itemCount(itemEntity->item.count),
        auxValue(itemEntity->item.getAuxValue()),
        x(itemEntity->x),
		y(itemEntity->y),
		z(itemEntity->z),
        _xa((signed char) (itemEntity->xd * 128.0)),
        _ya((signed char) (itemEntity->yd * 128.0)),
        _za((signed char) (itemEntity->zd * 128.0))
	{
    }

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(id);
		bitStream->Read(itemId);
		bitStream->Read(itemCount);
		bitStream->Read(auxValue);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(_xa);
		bitStream->Read(_ya);
		bitStream->Read(_za);
    }

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADDITEMENTITY));
        bitStream->Write(id);
        bitStream->Write(itemId);
        bitStream->Write(itemCount);
        bitStream->Write(auxValue);
        bitStream->Write(x);
        bitStream->Write(y);
        bitStream->Write(z);
        bitStream->Write(_xa);
        bitStream->Write(_ya);
        bitStream->Write(_za);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (AddItemEntityPacket*)this);
	}

	float xa() { return (float)(_xa) / 128.0f; }
	float ya() { return (float)(_ya) / 128.0f; }
	float za() { return (float)(_za) / 128.0f; }

    int id;
    float x, y, z;

    short itemId;
	short auxValue;
    unsigned char itemCount;
private:
	signed char _xa, _ya, _za;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__AddItemEntityPacket_H__*/
