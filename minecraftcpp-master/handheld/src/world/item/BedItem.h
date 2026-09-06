#ifndef NET_MINECRAFT_WORLD_ITEM__BedItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BedItem_H__
#include "Item.h"
class Player;
class ItemInstance;
class Level;
class BedItem : public Item{
	typedef Item super;
public:
	BedItem(int id) : super(id) {}
	bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
};

#endif /* NET_MINECRAFT_WORLD_ITEM__BedItem_H__ */