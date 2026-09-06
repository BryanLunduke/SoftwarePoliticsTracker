#ifndef NET_MINECRAFT_NETWORK_PACKET__LoginStatusPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__LoginStatusPacket_H__

#include "../Packet.h"

namespace LoginStatus {
	const int Success = 0;
	const int Failed_ClientOld = 1;
	const int Failed_ServerOld = 2;
}

class LoginStatusPacket : public Packet {
public:
	LoginStatusPacket()
	{
	}
	LoginStatusPacket(int status)
	:	status(status)
	{}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_LOGINSTATUS));
		bitStream->Write(status);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(status);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (LoginStatusPacket*)this);
	}

	int status;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__LoginStatusPacket_H__*/
