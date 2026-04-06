#ifndef NET_MINECRAFT_NETWORK_PACKET__AddEntityPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AddEntityPacket_H__

#include "../Packet.h"
#include "../../world/entity/Entity.h"

class AddEntityPacket : public Packet
{
public:
	AddEntityPacket() {}

	AddEntityPacket(const Entity* e, int data = 0)
	:	entityId(e->entityId),
		type(e->getEntityTypeId()),
		x(e->x),
		y(e->y),
		z(e->z),
		_data(data)
	{
		if (hasMovementData()) {
			xd = e->xd;
			yd = e->yd;
			zd = e->zd;
		}
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADDENTITY));
		bitStream->Write(entityId);
		unsigned char bType = (unsigned char)type;
		bitStream->Write(bType);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(_data);

		if (hasMovementData()) {
			const float M = 3.9f;
			short xa = (short)(8000.0f * Mth::clamp(xd, -M, M));
			short ya = (short)(8000.0f * Mth::clamp(yd, -M, M));
			short za = (short)(8000.0f * Mth::clamp(zd, -M, M));

			bitStream->Write(xa);
			bitStream->Write(ya);
			bitStream->Write(za);
		}
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		unsigned char bType;
		bitStream->Read(bType);
		type = bType;
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		bitStream->Read(_data);

		if (hasMovementData()) {
			short xa, ya, za;
			bitStream->Read(xa);
			bitStream->Read(ya);
			bitStream->Read(za);
			xd = xa / 8000.0f;
			yd = ya / 8000.0f;
			zd = za / 8000.0f;
		}
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (AddEntityPacket*)this);
	}

	bool hasMovementData() {
		return _data > 0;
	}
	int data() {
		return _data;
	}

public:
	int entityId;
	float x, y, z;
	float xd, yd, zd;
	int type;
private:
	int _data;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__AddEntityPacket_H__*/
