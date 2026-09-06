#ifndef NET_MINECRAFT_NETWORK_PACKET__AddMobPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AddMobPacket_H__

#include "../Packet.h"
#include "../../world/entity/Mob.h"
#include "../../util/RakDataIO.h"

class AddMobPacket : public Packet
{
public:
	AddMobPacket()
	:	_entityData(NULL)
	{
	}

	~AddMobPacket() {
		for (unsigned int i = 0; i < unpack.size(); ++i)
			delete unpack[i];
	}

	AddMobPacket(const Mob* mob)
	:	entityId(mob->entityId),
		type(mob->getEntityTypeId()),
		x(mob->x),
		y(mob->y),
		z(mob->z),
		xRot(mob->xRot),
		yRot(mob->yRot),
		_entityData(mob->getEntityData())
	{
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADDMOB));
		bitStream->Write(entityId);
		bitStream->Write(type);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(PacketUtil::Rot_degreesToChar(yRot));
		bitStream->Write(PacketUtil::Rot_degreesToChar(xRot));
		RakDataOutput dos(*bitStream);
		_entityData->packAll(&dos);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(type);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		char rx, ry;
		bitStream->Read(ry);
		bitStream->Read(rx);
		RakDataInput dis(*bitStream);
		unpack = SynchedEntityData::unpack(&dis);
		yRot = PacketUtil::Rot_degreesToChar(ry);
		xRot = PacketUtil::Rot_charToDegrees(rx);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (AddMobPacket*)this);
	}

public:
	int entityId;
	int type;
	float x, y, z;
	float xRot, yRot;
	SynchedEntityData::DataList unpack;
private:
	const SynchedEntityData* _entityData;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__AddMobPacket_H__*/
