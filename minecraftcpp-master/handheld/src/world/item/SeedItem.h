#ifndef NET_MINECRAFT_WORLD_ITEM__SeedItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SeedItem_H__

#include "Item.h"

class SeedItem : public Item {
	typedef Item super;
public:
	SeedItem(int id, int resultId, int targetLand)
	:	super(id),
		resultId(resultId),
		targetLand(targetLand)
	{}

	bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if(face != 1) return false;
		//if (!player.mayBuild(x, y, z) || !player.mayBuild(x, y + 1, z)) return false;
		int targetType = level->getTile(x, y, z);
		if(targetType == targetLand && level->isEmptyTile(x, y + 1, z)) {
			level->setTile(x, y + 1, z, resultId);
			itemInstance->count--;
			return true;
		}
		return false;
	}
private:
	int resultId;
	int targetLand;
};

#endif /* NET_MINECRAFT_WORLD_ITEM__SeedItem_H__ */
