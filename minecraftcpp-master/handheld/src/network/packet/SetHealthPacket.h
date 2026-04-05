#ifndef NET_MINECRAFT_NETWORK_PACKET__SetHealthPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetHealthPacket_H__

#include "../Packet.h"

//package net.minecraft.network.packet;

class SetHealthPacket: public Packet {
public:
	static const int HEALTH_MODIFY_OFFSET = -64;

    SetHealthPacket() {
    }

    SetHealthPacket(int health)
    :   health(health)
    {
    }

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETHEALTH));
		bitStream->Write((signed char)health);
	}

	void read(RakNet::BitStream* bitStream)
	{
		signed char tmpHealth;
		bitStream->Read(tmpHealth);
		health = tmpHealth;
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SetHealthPacket*)this);
	}

	int health;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SetHealthPacket_H__*/
