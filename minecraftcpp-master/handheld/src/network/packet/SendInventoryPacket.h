#ifndef NET_MINECRAFT_NETWORK_PACKET__SendInventoryPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__SendInventoryPacket_H__

#include "../Packet.h"

class SendInventoryPacket: public Packet
{
public:
	SendInventoryPacket()
	{
	}

	SendInventoryPacket(Player* player, bool dropItems)
	:	entityId(player->entityId),
		extra(dropItems? ExtraDrop : 0)
	{
        Inventory* inv = player->inventory;
		numItems = 0;
        for (int i = Inventory::MAX_SELECTION_SIZE; i < inv->getContainerSize(); ++i) {
			++numItems;
			ItemInstance* item = inv->getItem(i);
			items.push_back(item? *item : ItemInstance());
        }
        for (int i = 0; i < NumArmorItems; ++i) {
            ItemInstance* item = player->getArmor(i);
            items.push_back(item? *item : ItemInstance());
        }
	}

	void write(RakNet::BitStream* bitStream)
	{
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_SENDINVENTORY));
		bitStream->Write(entityId);
		bitStream->Write(extra);
		bitStream->Write(numItems);
        // Inventory
		for (int i = 0; i < numItems; ++i)
			PacketUtil::writeItemInstance(items[i], bitStream);
        // Armor
        for (int i = 0; i < NumArmorItems; ++i)
            PacketUtil::writeItemInstance(items[i + numItems], bitStream);
	}

	void read(RakNet::BitStream* bitStream)
	{
		bitStream->Read(entityId);
		bitStream->Read(extra);
		bitStream->Read(numItems);
		items.clear();
        // Inventory, Armor
		for (int i = 0; i < numItems + NumArmorItems; ++i)
			items.push_back(PacketUtil::readItemInstance(bitStream));
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback)
	{
		callback->handle(source, (SendInventoryPacket*)this);
	}

    int entityId;
	std::vector<ItemInstance> items;
	short numItems;
	unsigned char extra;

	static const int ExtraDrop = 1;
    static const int NumArmorItems = 4;
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__SendInventoryPacket_H__*/
