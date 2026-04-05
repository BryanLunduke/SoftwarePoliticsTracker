#include "PigZombie.h"
#include "../../item/Item.h"
#include "../../level/Level.h"
#include "../../Difficulty.h"
#include "../Entity.h"
#include "../projectile/Arrow.h"
PigZombie::PigZombie( Level* level )
	: super(level)
	,  angerTime(0)
	,  playAngrySoundIn(0)
	, weapon(Item::sword_gold)
    , stunedTime(SharedConstants::TicksPerSecond * 3){
	textureName = "mob/pigzombie.png";
	runSpeed = 0.7f;
	fireImmune = true;
	attackDamage = 5;
}

bool PigZombie::useNewAi() {
	return false;
}

void PigZombie::tick() {
	//runSpeed = attackTargetId != 0 ? 0.95f : 0.5f;
	if(stunedTime > 0)
		stunedTime--;
	if(playAngrySoundIn > 0) {
		if(--playAngrySoundIn == 0) {
			level->playSound(x, y, z, "mob.zombiepig.zpigangry", getSoundVolume() * 2, ((random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f) * 1.8f);
		}
	}
	super::tick();
}

std::string PigZombie::getTexture() {
	return "mob/pigzombie.png";
}

bool PigZombie::canSpawn() {
	return level->difficulty > Difficulty::PEACEFUL && level->isUnobstructed(bb) && level->getCubes(this, bb).empty();
}

void PigZombie::addAdditonalSaveData( CompoundTag* entityTag ) {
	super::addAdditonalSaveData(entityTag);
	entityTag->putShort("Anger", (short)angerTime);
}

void PigZombie::readAdditionalSaveData( CompoundTag* tag ) {
	super::readAdditionalSaveData(tag);
	angerTime = tag->getShort("Anger");
}

Entity* PigZombie::findAttackTarget() {
	if(stunedTime != 0)
		return NULL;
	if(angerTime == 0) {
		Entity* potentialTarget = super::findAttackTarget();
		if(potentialTarget != NULL && potentialTarget->distanceTo(x, y, z) < 5) {
			return potentialTarget;
		}
		return NULL;
	}
	return super::findAttackTarget();
}

bool PigZombie::hurt( Entity* sourceEntity, int dmg ) {
	Entity* attacker = NULL;
	if(sourceEntity != NULL) {
		if(sourceEntity->isPlayer()) {
			attacker = sourceEntity;
		}
		else if(sourceEntity->isEntityType(EntityTypes::IdArrow)) {
			Arrow* arrow = (Arrow*)sourceEntity;
			if(arrow->ownerId != 0) {
				attacker = level->getEntity(arrow->ownerId);
				if(!attacker->isPlayer())
					attacker = NULL;
			}
		}
	}
	
	if(attacker != NULL) {
		EntityList nearby =  level->getEntities(this, bb.grow(12, 12, 12));
		for(EntityList::iterator it = nearby.begin(); it != nearby.end(); ++it) {
			if((*it)->isEntityType(MobTypes::PigZombie)) {
				PigZombie* pigZombie = (PigZombie*)(*it);
				pigZombie->alert(sourceEntity);
			}
		}
	}
	return super::hurt(sourceEntity, dmg);
}

void PigZombie::alert( Entity* target ) {
	attackTargetId = target->entityId;
	angerTime = 20 * 20 * random.nextInt(20 * 20);
	playAngrySoundIn = random.nextInt(20 * 2);
}

const char* PigZombie::getAmbientSound() {
	return "mob.zombiepig.zpig";
}

std::string PigZombie::getHurtSound() {
	return "mob.zombiepig.zpighurt";
}

std::string PigZombie::getDeathSound() {
	return "mob.zombiepig.zpigdeath";
}

void PigZombie::dropDeathLoot() {
	int count = random.nextInt(2);
	for(int i = 0; i < count; ++i) {
		// We really should spawn gold nuggets instead of ingots.
		spawnAtLocation(Item::goldIngot->id, 1);
	}
}

bool PigZombie::interact( Player* player ) {
	return false;
}

int PigZombie::getDeathLoot() {
	return 0;
}

ItemInstance* PigZombie::getCarriedItem() {
	return &weapon;
}

int PigZombie::getEntityTypeId() const {
	return MobTypes::PigZombie;
}

int PigZombie::getAttackTime() {
	return 40;
}
