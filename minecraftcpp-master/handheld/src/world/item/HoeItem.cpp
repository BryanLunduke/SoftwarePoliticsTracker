#include "HoeItem.h"
#include "../level/tile/Tile.h"
#include "../level/Level.h"
#include "../entity/item/ItemEntity.h"

HoeItem::HoeItem( int id, Tier tier ) : super(id), tier(tier) {
	maxStackSize = 1;
	setMaxDamage(tier.getUses());
}

bool HoeItem::useOn( ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ ) {
	//if (!player.mayBuild(x, y, z)) return false;

	int targetType = level->getTile(x, y, z);
	int above = level->getTile(x, y + 1, z);
	
	if (face != 0 && above == 0 && targetType == Tile::grass->id || targetType == Tile::dirt->id) {
		Tile* tile = Tile::farmland;
		level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, tile->soundType->getStepSound(), (tile->soundType->getVolume() + 1) / 2, tile->soundType->getPitch() * 0.8f);
		itemInstance->hurt(1/*, player*/);
		if (level->isClientSide) return true;
		level->setTile(x, y, z, tile->id);
		if(targetType == Tile::grass->id && level->random.nextInt( 8 ) == 0) {
			float s = 0.7f;
			float xo = level->random.nextFloat() * s + (1 - s) * 0.5f;
			float yo = level->random.nextFloat() * s + (1 - s) * 2.5f;
			float zo = level->random.nextFloat() * s + (1 - s) * 0.5f;
			ItemEntity* item = new ItemEntity(level, float(x) + xo, float(y) + yo, float(z) + zo, ItemInstance(Item::seeds_wheat));
			item->throwTime = 10;
			level->addEntity(item);
		}
		return true;
	}

	return false;
}

bool HoeItem::isHandEquipped() const {
	return true;
}
