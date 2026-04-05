#ifndef NET_MINECRAFT_WORLD_ITEM__HangingEntityItem_H__
#define NET_MINECRAFT_WORLD_ITEM__HangingEntityItem_H__
#include "Item.h"
class HangingEntity;
class HangingEntityItem : public Item {
	typedef Item super;
public:
	HangingEntityItem(int id, int type);
	bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ);
private:
	HangingEntity* createEntity(Level* level, int x, int y, int z, int dir);
private:
	int entityType;
};

#endif /* NET_MINECRAFT_WORLD_ITEM__HangingEntityItem_H__ */