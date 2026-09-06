#ifndef NET_MINECRAFT_NETWORK_PACKET__ContainerOpenPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__ContainerOpenPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class ContainerOpenPacket: public Packet
{
public:
    ContainerOpenPacket() {
    }

    ContainerOpenPacket(int containerId, int type, const std::string& title, int size)
    :   containerId(containerId),
        type(type),
        title(title.c_str()),
        size(size)
    {
    }

    void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_CONTAINEROPEN));
        bitStream->Write(containerId);
        bitStream->Write(type);
        bitStream->Write(size);
        bitStream->Write(title);
    }

	void read(RakNet::BitStream* bitStream) {
        bitStream->Read(containerId);
        bitStream->Read(type);
        bitStream->Read(size);
        bitStream->Read(title);
    }

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (ContainerOpenPacket*)this);
	}

    RakNet::RakString title;
    unsigned char containerId;
    unsigned char type;
    unsigned char size;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__ContainerOpenPacket_H__*/
