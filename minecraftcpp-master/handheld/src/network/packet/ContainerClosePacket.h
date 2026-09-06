#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerClosePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerClosePacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class ContainerClosePacket: public Packet
{
public:
    ContainerClosePacket() {
    }

    ContainerClosePacket(int containerId)
    :   containerId(containerId)
    {
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
    }

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINERCLOSE));
        bitStream->Write(containerId);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerClosePacket*)this);
	}

    unsigned char containerId;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerClosePacket_H__*/
