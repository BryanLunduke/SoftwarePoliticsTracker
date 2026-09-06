#include "Monster.h"

#include "../player/Player.h"
#include "../../level/Level.h"
#include "../../Difficulty.h"
#include "../../../util/Mth.h"
//#include "../../effect/MobEffect.h"

Monster::Monster(Level* level)
:	super(level),
	attackDamage(2),
	targetId(0),
	lastHurtByMobId(0)
{
	entityRendererId = ER_HUMANOID_RENDERER;
	//xpReward = Enemy.XP_REWARD_MEDIUM;
}

void Monster::aiStep()
{
	updateAttackAnim();

	float br = getBrightness(1);
	if (br > 0.5f) {
		noActionTime += 2;
	}
	super::aiStep();
}

void Monster::tick()
{
	super::tick();
	if (!level->isClientSide && level->difficulty == Difficulty::PEACEFUL) remove();
}

bool Monster::hurt( Entity* sourceEntity, int dmg )
{
	if (super::hurt(sourceEntity, dmg)) {
		if (sourceEntity != this) {
			attackTargetId = 0;
			if (sourceEntity != NULL) {
				attackTargetId = sourceEntity->entityId;
				if (sourceEntity->isMob())
					lastHurtByMobId = sourceEntity->entityId;
			}
		}
		return true;
	}
	return false;
}

bool Monster::canSpawn()
{
	return isDarkEnoughToSpawn() && super::canSpawn();
}

Entity* Monster::findAttackTarget()
{
	//Player* player = level->getNearestAttackablePlayer(this, 16);
	Player* player = level->getNearestPlayer(this, 16);
	//LOGI("playuer: %p\n", player);
	if (player != NULL && canSee(player)) return player;
	return NULL;
}

bool Monster::isDarkEnoughToSpawn()
{
	//static int st = getTimeMs();
	//return getTimeMs() - st > 10000;

	int xt = Mth::floor(x);
	int yt = Mth::floor(bb.y0);
	int zt = Mth::floor(z);
	if (level->getBrightness(LightLayer::Sky, xt, yt, zt) > random.nextInt(32)) return false;

	int br = level->getRawBrightness(xt, yt, zt);

	//if (level->isThundering()) {
	//	int tmp = level->skyDarken;
	//	level->skyDarken = 10;
	//	br = level->getRawBrightness(xt, yt, zt);
	//	level->skyDarken = tmp;
	//}
	//LOGI("br: %d\n", br);
	return br <= random.nextInt(8); // was 8
}

bool Monster::doHurtTarget( Entity* target )
{
	swing();
	//if (target->isMob()) setLastHurtMob(target);
	//@todo
	int dmg = attackDamage;
	//if (hasEffect(MobEffect.damageBoost)) {
	//    dmg += (3 << getEffect(MobEffect.damageBoost).getAmplifier());
	//}
	//if (hasEffect(MobEffect.weakness)) {
	//    dmg -= (2 << getEffect(MobEffect.weakness).getAmplifier());
	//}

	return target->hurt(this, dmg);
}

void Monster::checkHurtTarget( Entity* target, float distance ) {
	if (attackTime <= 0 && distance < 2.0f && target->bb.y1 > bb.y0 && target->bb.y0 < bb.y1) {
		attackTime = getAttackTime();
		doHurtTarget(target);
	}
}

float Monster::getWalkTargetValue( int x, int y, int z )
{
	return 0.5f - level->getBrightness(x, y, z);
}

int Monster::getCreatureBaseType() const {
	return MobTypes::BaseEnemy;
}

Mob* Monster::getTarget() {
	if (targetId == 0) return NULL;
	return level->getMob(targetId);
}

void Monster::setTarget(Mob* mob) {
	targetId = mob? mob->entityId : 0;
}

Mob* Monster::getLastHurtByMob() {
	if (targetId == 0) return NULL;
	return level->getMob(lastHurtByMobId);
}

void Monster::setLastHurtByMob(Mob* mob) {
	lastHurtByMobId = mob? mob->entityId : 0;
}

int Monster::getAttackDamage( Entity* target ) {
	return attackDamage;
}

int Monster::getAttackTime() {
	return 20;
}
