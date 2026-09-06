#ifndef NET_MINECRAFT_NETWORK_PACKET__LevelEventPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__LevelEventPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class LevelEventPacket: public Packet {
public:
    LevelEventPacket() {}

    LevelEventPacket(int eventId, int x, int y, int z, int data)
    :   eventId(eventId),
        x(x),
        y(y),
        z(z),
        data(data)
    {}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_LEVELEVENT));
		bitStream->Write(eventId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(data);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(eventId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(data);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (LevelEventPacket*)this);
	}

    short eventId;
    short x, y, z;
    int data;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__LevelEventPacket_H__*/
