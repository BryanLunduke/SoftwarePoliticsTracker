#include "Creeper.h"
#include "../Entity.h"
#include "../../item/Item.h"
#include "../../level/Level.h"
#include "../../../nbt/CompoundTag.h"

Creeper::Creeper( Level* level )
:	super(level),
	swell(0),
	oldSwell(0),
	swellDir(-1)
{
	entityRendererId = ER_CREEPER_RENDERER;
	this->textureName = "mob/creeper.png";

	entityData.define(DATA_SWELL_DIR, (SynchedEntityData::TypeChar) -1);
}

int Creeper::getMaxHealth() {
	return 16;
}

void Creeper::tick() {
	oldSwell = swell;
	if (level->isClientSide) {
		int swellDir = getSwellDir();
		if (swellDir > 0 && swell == 0) {
			level->playSound(this, "random.fuse", 1, 0.5f);
		}
		swell += swellDir;
		if (swell < 0) swell = 0;
		if (swell >= MAX_SWELL) swell = MAX_SWELL;
	}
	super::tick();

	if (!level->isClientSide && attackTargetId == 0) {
		if (swell > 0) {
			setSwellDir(-1);
			swell--;
			if (swell < 0) {
				swell = 0;
			}
		}
	}
}

float Creeper::getSwelling( float a ) {
	return (oldSwell + (swell - oldSwell) * a) / (MAX_SWELL - 2);
}

int Creeper::getEntityTypeId() const {
	return MobTypes::Creeper;
}

int Creeper::getDeathLoot() {
	return Item::sulphur->id;
}

void Creeper::checkCantSeeTarget( Entity* target, float d ) {
	if (level->isClientSide) return;
	if (swell > 0) {
		setSwellDir(-1);
		swell--;
		if (swell < 0) {
			swell = 0;
		}
	}
}

std::string Creeper::getHurtSound() {
	return "mob.creeper";
}

std::string Creeper::getDeathSound() {
	return "mob.creeperdeath";
}

void Creeper::checkHurtTarget( Entity* target, float d ) {
	if (level->isClientSide) return;
	const int swellDir = getSwellDir();
	if ((swellDir <= 0 && d < 3) || (swellDir > 0 && d < 7)) {
		if (swell == 0) {
			level->playSound(this, "random.fuse", 1, 0.5f);
		}
		setSwellDir(1);

		if (++swell >= MAX_SWELL) {
			level->explode(this, x, y, z, 2.4f);
			remove();
		}
		holdGround = true;
	} else {
		setSwellDir(-1);
		if (--swell < 0)
			swell = 0;
	}
}

int Creeper::getSwellDir() {
	return (int) entityData.getByte(DATA_SWELL_DIR);
}

void Creeper::setSwellDir( int dir ) {
	entityData.set(DATA_SWELL_DIR, (SynchedEntityData::TypeChar) dir);
}

//@todo
//void die(DamageSource* source) {
//    super::die(source);

//    if (source.getEntity() instanceof Skeleton) {
//        spawnAtLocation(Item::record_01->id + random.nextInt(2), 1);
//    }
//}
