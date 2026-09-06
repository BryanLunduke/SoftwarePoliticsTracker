#ifndef NET_MINECRAFT_NETWORK_PACKET__SetEntityDataPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SetEntityDataPacket_H__

//package net.minecraft.network.packet;

#include "../Packet.h"

#include "../../world/entity/SynchedEntityData.h"
#include "../../util/RakDataIO.h"

class SetEntityDataPacket: public Packet
{
public:
    SetEntityDataPacket()
	:	deletePackedItems(false)
	{}

    SetEntityDataPacket(int id, SynchedEntityData& entityData)
	:	id(id),
		deletePackedItems(false),
		packedItems(entityData.packDirty())
	{
    }

	~SetEntityDataPacket() {
		if (deletePackedItems)
			for (unsigned int i = 0; i < packedItems.size(); ++i)
				delete packedItems[i];
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SETENTITYDATA));
		bitStream->Write(id);

		RakDataOutput dos(*bitStream);
		SynchedEntityData::pack(&packedItems, &dos);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(id);

		RakDataInput dis(*bitStream);
		packedItems = SynchedEntityData::unpack(&dis);
		deletePackedItems = true;
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SetEntityDataPacket*)this);
	}

    SynchedEntityData::DataList& getUnpackedData() {
        return packedItems;
    }
public:
    int id;
private:
	bool deletePackedItems;
    SynchedEntityData::DataList packedItems;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SetEntityDataPacket_H__*/
