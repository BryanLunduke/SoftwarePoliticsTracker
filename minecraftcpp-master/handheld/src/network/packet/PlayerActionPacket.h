#ifndef NET_MINECRAFT_NETWORK_PACKET__PlayerActionPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlayerActionPacket_H__

#include "../Packet.h"

class PlayerActionPacket : public Packet
{
public:
	static const int START_DESTROY_BLOCK = 0;
	static const int ABORT_DESTROY_BLOCK = 1;
	static const int STOP_DESTROY_BLOCK = 2;
	static const int GET_UPDATED_BLOCK = 3;
	static const int DROP_ITEM = 4;
	static const int RELEASE_USE_ITEM = 5;
	static const int STOP_SLEEPING = 6;

	PlayerActionPacket()
	:	x(0),
		y(0),
		z(0),
		action(0),
		face(0),
		entityId(0)
	{}

	PlayerActionPacket(int action, int x, int y, int z, int face, int entityId)
	:	x(x),
		y(y),
		z(z),
		face(face),
		action(action),
		entityId(entityId)
	{}

	void read(RakNet::BitStream* bitStream) {
		bitStream->Read(action);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(face);
		bitStream->Read(entityId);
	}

	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLAYERACTION));

		bitStream->Write(action);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(face);
		bitStream->Write(entityId);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (PlayerActionPacket*)this);
	}

	int x, y, z, face, action, entityId;
};

#endif /* NET_MINECRAFT_NETWORK_PACKET__PlayerActionPacket_H__ */
