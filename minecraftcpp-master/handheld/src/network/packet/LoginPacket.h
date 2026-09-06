#ifndef NET_MINECRAFT_NETWORK_PACKET__LoginPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__LoginPacket_H__

#include "../Packet.h"

class LoginPacket : public Packet
{
public:
	RakNet::RakString clientName;
	int clientNetworkVersion;
	int clientNetworkLowestSupportedVersion;

	LoginPacket()
	:	clientNetworkVersion(-1),
		clientNetworkLowestSupportedVersion(-1)
	{
	}

	LoginPacket(const RakNet::RakString& clientName, int clientVersion)
	:	clientName(clientName),
		clientNetworkVersion(clientVersion),
		clientNetworkLowestSupportedVersion(clientVersion)
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_LOGIN));
		bitStream->Write(clientName);
		bitStream->Write(clientNetworkVersion);
		bitStream->Write(clientNetworkLowestSupportedVersion);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(clientName);
		// First versions didn't send the client version
		//LOGI("unread: %d\n", bitStream->GetNumberOfUnreadBits());
		if (bitStream->GetNumberOfUnreadBits() > 0) {
			bitStream->Read(clientNetworkVersion);
			bitStream->Read(clientNetworkLowestSupportedVersion);
		}
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (LoginPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__LoginPacket_H__*/
