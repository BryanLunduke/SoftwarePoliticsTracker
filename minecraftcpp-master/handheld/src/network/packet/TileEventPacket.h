#ifndef NET_MINECRAFT_NETWORK_PACKET__TileEventPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__TileEventPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class TileEventPacket: public Packet {
public:
    TileEventPacket() {}

    TileEventPacket(int x, int y, int z, int b0, int b1)
    :   x(x),
        y(y),
        z(z),
        b0(b0),
        b1(b1)
    {}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_TILEEVENT));
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(b0);
		bitStream->Write(b1);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(b0);
		bitStream->Read(b1);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (TileEventPacket*)this);
	}

    int x, y, z;
    int b0, b1;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__TileEventPacket_H__*/
