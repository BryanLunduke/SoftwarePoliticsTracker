#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__PlayerData_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__PlayerData_H__

#include "../../entity/player/Player.h"
#include "../../entity/player/Inventory.h"
#include "../../phys/Vec3.h"
#include "../../../util/Mth.h"

static float clampRot(float r) {
	return Mth::clamp(r, -100000.0f, 100000.0f);
}
static float clampXZ(float r) {
	return Mth::clamp(r, 4.0f, 252.0f);
}
static float clampY(float r) {
	return Mth::clamp(r, 4.0f, 126.0f);
}

class PlayerData {
public:
	void loadPlayer(Player* p) const {
		p->setPos(0, 0, 0);

		p->x = p->xo = p->xOld = clampXZ(pos.x);
		p->y = p->yo = p->yOld = clampY (pos.y);
		p->z = p->zo = p->zOld = clampXZ(pos.z);

		float motionX = motion.x;
		float motionY = motion.y;
		float motionZ = motion.z;
		if (Mth::abs(motionX) > 10.0f) motionX = 0;
		if (Mth::abs(motionY) > 10.0f) motionY = 0;
		if (Mth::abs(motionZ) > 10.0f) motionZ = 0;

		p->xd = motionX;
		p->yd = motionY;
		p->zd = motionZ;

		p->xRot = p->xRotO = clampRot(xRot);
		p->yRot = p->yRotO = clampRot(yRot);

		p->fallDistance = fallDistance;
		p->onFire    = onFire;
		p->airSupply = airSupply;
		p->onGround  = onGround;

		p->setPos(pos.x, pos.y, pos.z);

		//@deprecated
		//for (int i = 0; i < Inventory::MAX_SELECTION_SIZE; ++i)
			//p->inventory->setSelectionSlotItemId(i, inventorySlots[i]);
	}

	Vec3 pos;
	Vec3 motion;
	float xRot, yRot;

	float fallDistance;
	short onFire;
	short airSupply;
	bool onGround;

	int inventorySlots[Inventory::MAX_SELECTION_SIZE];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__PlayerData_H__*/
