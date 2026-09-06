#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerSetDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerSetDataPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class ContainerSetDataPacket: public Packet
{
public:
    ContainerSetDataPacket() {
    }

    ContainerSetDataPacket(int containerId, int id, int value)
    :   containerId(containerId),
        id(id),
        value(value)
    {
    }

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINERSETDATA));
        bitStream->Write(containerId);
        bitStream->Write(id);
        bitStream->Write(value);
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
        bitStream->Read(id);
        bitStream->Read(value);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerSetDataPacket*)this);
	}

    short id;
    short value;
    unsigned char containerId;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerSetDataPacket_H__*/
