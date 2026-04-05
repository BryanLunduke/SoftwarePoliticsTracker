#ifndef NET_MINECRAFT_NETWORK_PACKET__UpdateArmorPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__UpdateArmorPacket_H__

#include "../Packet.h"

class UpdateArmorPacket: public Packet
{
public:
	UpdateArmorPacket(Player* player)
	{
	}

	void write(RakNet::BitStream* bitStream){
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_UPDATEARMOR));

		//bitStream->Write();
	}

	void read(RakNet::BitStream* bitStream) {
		//bitStream->Read();
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (UpdateArmorPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__UpdateArmorPacket_H__*/
