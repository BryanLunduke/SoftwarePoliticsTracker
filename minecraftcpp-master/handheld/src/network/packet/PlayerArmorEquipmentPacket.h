#ifndef NET_MINECRAFT_NETWORK_PACKET__PlayerArmorEquipmentPacket_H__
#define NET_MINECRAFT_NETWORK_PACKET__PlayerArmorEquipmentPacket_H__

#include "../Packet.h"
#include "../../world/entity/player/Player.h"
#include "../../world/item/ArmorItem.h"
#include "../../world/item/ItemInstance.h"

// @note: A visual update only
class PlayerArmorEquipmentPacket : public Packet
{
public:
	int entityId;
	signed char head;
	signed char torso;
	signed char legs;
	signed char feet;

	PlayerArmorEquipmentPacket() {
	}

	PlayerArmorEquipmentPacket(Player* player)
	:	entityId(player->entityId)
	{
		get(head,  player->getArmor(ArmorItem::SLOT_HEAD));
		get(torso, player->getArmor(ArmorItem::SLOT_TORSO));
		get(legs,  player->getArmor(ArmorItem::SLOT_LEGS));
		get(feet,  player->getArmor(ArmorItem::SLOT_FEET));
	}

	void write(RakNet::BitStream* bitStream) {
		bitStream->Write((RakNet::MessageID)(ID_USER_PACKET_ENUM + PACKET_PLAYERARMOREQUIPMENT));

		bitStream->Write(entityId);
		bitStream->Write(head);
		bitStream->Write(torso);
		bitStream->Write(legs);
		bitStream->Write(feet);
	}

	void read(RakNet::BitStream* bitStream) {
		bitStream->Read(entityId);
		bitStream->Read(head);
		bitStream->Read(torso);
		bitStream->Read(legs);
		bitStream->Read(feet);
	}

	void handle(const RakNet::RakNetGUID& source, NetEventCallback* callback) {
		callback->handle(source, (PlayerArmorEquipmentPacket*)this);
	}

	void fillIn(Player* player) {
		set(player, head,  ArmorItem::SLOT_HEAD);
		set(player, torso, ArmorItem::SLOT_TORSO);
		set(player, legs,  ArmorItem::SLOT_LEGS);
		set(player, feet,  ArmorItem::SLOT_FEET);
	}

private:
	void get(signed char& s, const ItemInstance* item) {
		if (item) {
			s = item->id - 256;
		} else {
			s = -1;
		}
	}

	void set(Player* p, signed char s, int slot) {
		if (s < 0) p->setArmor(slot, NULL);
		else {
			ItemInstance item((int)s + 256, 1, 0);
			p->setArmor(slot, &item);
		}
	}
};

#endif /*NET_MINECRAFT_NETWORK_PACKET__PlayerArmorEquipmentPacket_H__*/
