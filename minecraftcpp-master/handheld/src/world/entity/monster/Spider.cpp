#include "Spider.h"
#include "../../item/Item.h"
#include "../../level/Level.h"
#include "../../../util/Mth.h"

Spider::Spider( Level* level )
:   super(level),
	fireCheckTick(0)
{
	entityRendererId = ER_SPIDER_RENDERER;
	this->textureName = "mob/spider.png";

	this->setSize(1.4f, 0.9f);
	runSpeed = 0.5f;

	entityData.define(DATA_FLAGS_ID, (DataFlagIdType) 0);
}

void Spider::aiStep() {
	super::aiStep();
}

void Spider::tick() {
	super::tick();

	if (!level->isClientSide) {
		// this is to synchronize the spiders' climb state
		// in multiplayer (to stop them from "flashing")
		setClimbing(horizontalCollision);
	}
}

int Spider::getMaxHealth() {
	return 8; // 12
}

bool Spider::onLadder() {
	return isClimbing();
}

void Spider::makeStuckInWeb() {
	// do nothing - spiders don't get stuck in web
}

float Spider::getModelScale() {
	return 1.0f;
}

bool Spider::isClimbing() {
	return entityData.getFlag<DataFlagIdType>(DATA_FLAGS_ID, 0);
}

void Spider::setClimbing( bool value ) {
	if (value)
		return entityData.setFlag<DataFlagIdType>(DATA_FLAGS_ID, 0);
	else
		return entityData.clearFlag<DataFlagIdType>(DATA_FLAGS_ID, 0);
}

int Spider::getEntityTypeId() const {
	return MobTypes::Spider;
}

bool Spider::makeStepSound() {
	return false;
}

Entity* Spider::findAttackTarget() {
	float br = getBrightness(1);
	if (br < 0.5f) {
		return level->getNearestPlayer(this, 16);
	}
	return NULL;
}

const char* Spider::getAmbientSound() {
	return "mob.spider";
}

std::string Spider::getHurtSound() {
	return "mob.spider";
}

std::string Spider::getDeathSound() {
	return "mob.spiderdeath";
}

void Spider::checkHurtTarget( Entity* target, float d ) {
	float br = getBrightness(1);
	if (br > 0.5f && random.nextInt(100) == 0) {
		attackTargetId = 0;
		return;
	}

	if (d > 2 && d < 6 && random.nextInt(10) == 0) {
		if (onGround) {
			float xdd = target->x - x;
			float zdd = target->z - z;
			float dd = Mth::sqrt(xdd * xdd + zdd * zdd);
			xd = (xdd / dd * 0.5f) * 0.8f + xd * 0.2f;
			zd = (zdd / dd * 0.5f) * 0.8f + zd * 0.2f;
			yd = 0.4f;
		}
	} else {
		super::checkHurtTarget(target, d);
	}
}

int Spider::getDeathLoot() {
	return Item::string->id;
}

//void dropDeathLoot(/*bool wasKilledByPlayer, int playerBonusLevel*/) {
//    super::dropDeathLoot(/*wasKilledByPlayer, playerBonusLevel*/);

////    if (wasKilledByPlayer && (random.nextInt(3) == 0 || random.nextInt(1 + playerBonusLevel) > 0)) {
////        spawnAtLocation(Item::spiderEye->id, 1);
////    }
//}

//float getRideHeight() {
//    return bbHeight * .75f - 0.5f;
//}

/*@Override*/
//     MobType getMobType() {
//         return MobType.ARTHROPOD;
//     }

/*@Override*/ //@todo
//     bool canBeAffected(MobEffectInstance newEffect) {
//         if (newEffect.getId() == MobEffect.poison.id) {
//             return false;
//         }
//         return super::canBeAffected(newEffect);
//     }
// 
