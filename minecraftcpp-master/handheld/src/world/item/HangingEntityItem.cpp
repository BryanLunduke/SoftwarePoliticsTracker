#include "HangingEntityItem.h"
#include "../entity/HangingEntity.h"
#include "../Facing.h"
#include "../Direction.h"
#include "../level/Level.h"
#include "../entity/EntityFactory.h"
#include "../entity/Painting.h"
HangingEntityItem::HangingEntityItem( int id, int type ) : super(id), entityType(type) {

}

bool HangingEntityItem::useOn( ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ ) {
	if (face == Facing::DOWN) return false;
	if (face == Facing::UP) return false;

	int dir = Direction::FACING_DIRECTION[face];

	HangingEntity* entity = createEntity(level, x, y, z, dir);
	//if (!player->mayUseItemAt(xt, yt, zt, face, instance)) return false;
	if(entity != NULL) {
		if (entity->survives()) {
			if (!level->isClientSide) {
				level->addEntity(entity);
			}
			else {
				delete entity;
			}
			itemInstance->count--;
		}
		else {
			delete entity;
		}
	}
	return true;
}

HangingEntity* HangingEntityItem::createEntity( Level* level, int x, int y, int z, int dir ) {
	switch(entityType) {
	case EntityTypes::IdPainting: return new Painting(level, x, y, z, dir);
	}
	return NULL;
}

