#ifndef NET_MINECRAFT_NETWORK_PACKET__SetTimePacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetTimePacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

class SetTimePacket: public Packet {
public:
    SetTimePacket() {
    }

    SetTimePacket(long time)
	:	time(time)
	{}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETTIME));
		bitStream->Write(time);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(time);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SetTimePacket*)this);
	}
	
	long time;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SetTimePacket_H__*/
