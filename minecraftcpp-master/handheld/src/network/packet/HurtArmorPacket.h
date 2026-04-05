#ifndef NET_MINECRAFT_NETWORK_PACKET__HurtArmorPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__HurtArmorPacket_H__

#include "../Packet.h"

class HurtArmorPacket: public Packet
{
public:
    HurtArmorPacket() {}

    HurtArmorPacket(int dmg)
    :   dmg(dmg)
    {}

	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_HURTARMOR));

		bitStream->Write(dmg);
	}

	void read(RakNet::BitStream* bitStream) {
		bitStream->Read(dmg);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (HurtArmorPacket*)this);
	}

    signed char dmg;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__HurtArmorPacket_H__*/
