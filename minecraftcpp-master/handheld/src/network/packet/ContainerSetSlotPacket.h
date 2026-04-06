#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerSetSlotPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerSetSlotPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"
#include "../../world/item/ItemInstance.h"

// Note: This can be seen as "ContainerWantSetSlotPacket" when sent from
//       client to server. Currently, the client handles side-effects relating
//       to it's own inventory, regardless of the success of the operation.
class ContainerSetSlotPacket: public Packet
{
public:
	static const char SETTYPE_SET = 0;
	static const char SETTYPE_ADD = 1;
	//static const int SETTYPE_SUB = 2;
	static const char SETTYPE_TAKE = 5;

    ContainerSetSlotPacket() {
    }

	//@todo: pointer parameter?
    ContainerSetSlotPacket(int containerId, int slot, const ItemInstance& item)
    :   containerId(containerId),
        slot(slot),
        item(item),
		setType(SETTYPE_SET)
		//item(item? *item : ItemInstance())
    {
    }
	ContainerSetSlotPacket(char setType, int containerId, int slot, const ItemInstance& item)
	:   setType(setType),
		containerId(containerId),
		slot(slot),
		item(item)
		//item(item? *item : ItemInstance())
	{
	}

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINERSETSLOT));
        bitStream->Write(containerId);
        bitStream->Write(slot);
		PacketUtil::writeItemInstance(item, bitStream);
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
        bitStream->Read(slot);
		item = PacketUtil::readItemInstance(bitStream);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerSetSlotPacket*)this);
	}

	unsigned char setType;
    unsigned char containerId;
    short slot;
    ItemInstance item;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerSetSlotPacket_H__*/
