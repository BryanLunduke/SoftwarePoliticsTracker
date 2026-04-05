#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerAckPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerAckPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class ContainerAckPacket: public Packet
{
public:
    ContainerAckPacket() {
    }

    ContainerAckPacket(int containerId, short uid, bool accepted)
    :   containerId(containerId),
        uid(uid),
        accepted(accepted)
    {
    }

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINERACK));
        bitStream->Write(containerId);
        bitStream->Write(uid);
        bitStream->Write(accepted);
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
        bitStream->Read(uid);
        bitStream->Read(accepted);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerAckPacket*)this);
	}

    short uid;
    unsigned char containerId;
    bool accepted;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerAckPacket_H__*/
