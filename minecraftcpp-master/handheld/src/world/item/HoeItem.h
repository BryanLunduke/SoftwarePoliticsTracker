#ifndef NET_MINECRAFT_WORLD_ITEM__HoeItem_H__
#define NET_MINECRAFT_WORLD_ITEM__HoeItem_H__

#include "Item.h"
class HoeItem : public Item
{
	typedef Item super;
public:
	HoeItem(int id, Tier tier);
	bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
	bool isHandEquipped() const;

protected:
	Tier tier;
};

#endif /* NET_MINECRAFT_WORLD_ITEM__HoeItem_H__ */