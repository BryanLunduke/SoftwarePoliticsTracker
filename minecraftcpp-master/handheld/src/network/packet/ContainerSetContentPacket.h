#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerSetContentPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerSetContentPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class ContainerSetContentPacket: public Packet
{
public:
    ContainerSetContentPacket() {
    }

    ContainerSetContentPacket(int containerId, const std::vector<ItemInstance>& newItems)
    :   containerId(containerId),
        items(newItems.begin(), newItems.end())
    {
    }

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINERSETCONTENT));
        bitStream->Write(containerId);
        bitStream->Write((short)items.size());
        for (unsigned int i = 0; i < items.size(); ++i)
    		PacketUtil::writeItemInstance(items[i], bitStream);
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
        short numItems;
        bitStream->Read(numItems);
        for (int i = 0; i < numItems; ++i)
            items.push_back( PacketUtil::readItemInstance(bitStream) );
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerSetContentPacket*)this);
	}

    unsigned char containerId;
    std::vector<ItemInstance> items;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerSetContentPacket_H__*/
