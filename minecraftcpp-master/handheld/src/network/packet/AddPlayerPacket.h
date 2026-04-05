#ifndef NET_MINECRAFT_NETWORK_PACKET__AddPlayerPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__AddPlayerPacket_H__

#include "../Packet.h"
#include "../../world/entity/player/Player.h"
#include "../../world/entity/player/Inventory.h"

class AddPlayerPacket : public Packet
{
public:
	AddPlayerPacket()
	:	_entityData(NULL)
	{}

	AddPlayerPacket(const Player* p)
	:	owner(p->owner),
		name(p->name.c_str()),
		entityId(p->entityId),
		x(p->x),
		y(p->y - p->heightOffset),
		z(p->z),
		xRot(p->xRot),
		yRot(p->yRot),
		carriedItemId(0),
		carriedItemAuxValue(0),
		_entityData(p->getEntityData())
	{
		if (ItemInstance* item = p->inventory->getSelected()) {
			carriedItemId       = item->id;
			carriedItemAuxValue = item->getAuxValue();
		}
	}

	~AddPlayerPacket() {
		for (unsigned int i = 0; i < unpack.size(); ++i)
			delete unpack[i];
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_ADDPLAYER));

		bitStream->Write(owner);
		bitStream->Write(name);
		bitStream->Write(entityId);
		bitStream->Write(x);
		bitStream->Write(y);
		bitStream->Write(z);
		bitStream->Write(PacketUtil::Rot_degreesToChar(yRot));
		bitStream->Write(PacketUtil::Rot_degreesToChar(xRot));
		bitStream->Write(carriedItemId);
		bitStream->Write(carriedItemAuxValue);
		RakDataOutput dos(*bitStream);
		_entityData->packAll(&dos);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(owner);
		bitStream->Read(name);
		bitStream->Read(entityId);
		bitStream->Read(x);
		bitStream->Read(y);
		bitStream->Read(z);
		char rx, ry;
		bitStream->Read(ry);
		bitStream->Read(rx);
		bitStream->Read(carriedItemId);
		bitStream->Read(carriedItemAuxValue);
		RakDataInput dis(*bitStream);
		unpack = SynchedEntityData::unpack(&dis);
		yRot = PacketUtil::Rot_degreesToChar(ry);
		xRot = PacketUtil::Rot_charToDegrees(rx);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (AddPlayerPacket*)this);
	}

	RakNet::RakNetGUID owner;
	RakNet::RakString name;
	int entityId;
	float x, y, z;
	float xRot, yRot;
	short carriedItemId;
	short carriedItemAuxValue;
	SynchedEntityData::DataList unpack;
private:
	const SynchedEntityData* _entityData;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__AddPlayerPacket_H__*/
