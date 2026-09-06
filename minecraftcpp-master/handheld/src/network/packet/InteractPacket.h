#ifndef NET_MINECRAFT_NETWORK_PACKET__InteractPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__InteractPacket_H__

#include "../Packet.h"

class InteractPacket : public Packet
{
public:
    static const int Interact = 1;
    static const int Attack = 2;

    char action;
    int sourceId;
    int targetId;

	InteractPacket() {
    }

	InteractPacket(char action, int sourceId, int targetId)
	:	action(action),
		sourceId(sourceId),
		targetId(targetId)
	{}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_INTERACT));

		bitStream->Write(action);
		bitStream->Write(sourceId);
		bitStream->Write(targetId);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(action);
		bitStream->Read(sourceId);
		bitStream->Read(targetId);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (InteractPacket*)this);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__InteractPacket_H__*/
