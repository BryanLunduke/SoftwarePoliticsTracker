#ifndef NET_MINECRAFT_NETWORK_PACKET__MoveEntityPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__MoveEntityPacket_H__

#include "../Packet.h"

class MoveEntityPacket: public Packet
{
public:
	MoveEntityPacket()
	:	hasRot(false)
	{}

	// PACKET_MOVEENTITY is unknown and undefined (and not used)
	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_MOVEENTITY));
	}

	void read(RakNet::BitStream* bitStream) {};

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (MoveEntityPacket*)this);
	}

	int entityId;
	float x, y, z;
	float xRot;
	float yRot;
	bool hasRot;
};

class MoveEntityPacket_PosRot: public MoveEntityPacket
{
	typedef MoveEntityPacket super;
public:
	MoveEntityPacket_PosRot() {
		hasRot = true;
	}

	//MoveEntityPacket_PosRot(int entityId, float x, float y, float z, float yRot, float xRot) {
	//	set(entityId, x, y, z, yRot, xRot);
	//}

	MoveEntityPacket_PosRot(const Entity* e) {
		set(e->entityId, e->x, e->y - e->heightOffset, e->z, e->yRot, e->xRot);
	}

	void set(int entityId, float x, float y, float z, float yRot, float xRot) {
		this->entityId = entityId;
		this->x = x;
		this->y = y;
		this->z = z;
		this->xRot = xRot;
		this->yRot = yRot;
	}

	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_MOVEENTITY_POSROT));

		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(yRot);
		bitStream->Write(xRot);
	}

	void read(RakNet::BitStream* bitStream) {
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(yRot);
		bitStream->Read(xRot);
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__MoveEntityPacket_H__*/
