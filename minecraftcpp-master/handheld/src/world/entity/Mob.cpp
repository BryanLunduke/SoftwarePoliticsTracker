#include "Mob.h"

#include <cmath>

#include "player/Player.h"
#include "SharedFlags.h"
#include "../entity/EntityEvent.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"
#include "../../util/Random.h"
#include "../../util/PerfTimer.h"

#include "../../nbt/CompoundTag.h"

#include "../../network/RakNetInstance.h"
#include "../../network/packet/MoveEntityPacket.h"
#include "ai/control/MoveControl.h"
#include "ai/control/JumpControl.h"
#include "ai/PathNavigation.h"
#include "ai/Sensing.h"
#include "ai/goal/GoalSelector.h"
#include "../../network/packet/SetEntityMotionPacket.h"
#include "../item/ArmorItem.h"


Mob::Mob(Level* level)
:	super(level),
	invulnerableDuration(20),
	//hasHair(false),
	textureName("mob/char.png"),
	allowAlpha(true),
	modelName(""),
	bobStrength(1),
	deathScore(0),
	renderOffset(0),
	//interpolateOnly(false),
	lookingAtId(0),
	jumping(false),
	defaultLookAngle(0.0f),
	runSpeed(0.7f),
	walkingSpeed(0.1f),
	flyingSpeed(0.02f),
	run(0), oRun(0),
	animStep(0), animStepO(0),
	lx(0), ly(0), lz(0), lxr(0), lyr(0),
	lSteps(0),
	walkAnimSpeed(0), walkAnimSpeedO(0), walkAnimPos(0),
	attackAnim(0), oAttackAnim(0),

	autoSendPosRot(true),
	sentX(0), sentY(0), sentZ(0),
	sentRotX(0), sentRotY(0),
	sentXd(0), sentYd(0), sentZd(0),
	arrowCount(0),
	removeArrowTime(0),
	speed(0),
	moveControl(NULL),
	jumpControl(NULL),
	navigation(NULL),
	goalSelector(NULL),
	targetSelector(NULL),
	sensing(NULL),
	swinging(false),
	swingTime(0),
	lastHurt(0),
	dmgSpill(0),
	bypassArmor(false)
{
	entityData.define(SharedFlagsInformation::DATA_SHARED_FLAGS_ID, (SynchedEntityData::TypeChar) 0);
	entityData.define(DATA_AIR_SUPPLY_ID, (SynchedEntityData::TypeShort) TOTAL_AIR_SUPPLY);

	_init();
	health = getMaxHealth();

	blocksBuilding = true;
	ambientSoundTime = -level->random.nextInt(100);

	rotA = (float) (Mth::random() + 1) * 0.01f;
	setPos(x, y, z);
	timeOffs = (float) Mth::random() * 12398;
	yRot = (float) (Mth::random() * Mth::PI * 2);

	this->footSize = 0.5f;
}

Mob::~Mob() {
	//LOGI("destroying an entity! %p\n", this);
}

float Mob::getWalkingSpeedModifier() {
	float speed = 0.7f; // is 1.0f in desktop Minecraft

	//if (hasEffect(MobEffect.movementSpeed)) {
	//	speed *= 1.0f + .2f * (getEffect(MobEffect.movementSpeed).getAmplifier() + 1);
	//}
	//if (hasEffect(MobEffect.movementSlowdown)) {
	//	speed *= 1.0f - .15f * (getEffect(MobEffect.movementSlowdown).getAmplifier() + 1);
	//}
	return speed;
}

bool Mob::canSee( Entity* target )
{
	return !level->clip(Vec3(x, y + getHeadHeight(), z), Vec3(target->x, target->y + target->getHeadHeight(), target->z)).isHit();
}

std::string Mob::getTexture()
{
	return textureName;
}

bool Mob::isPickable()
{
	return !removed;
}

bool Mob::isPushable()
{
	return !removed;
}

float Mob::getHeadHeight()
{
	return bbHeight * 0.85f;
}

float Mob::getVoicePitch() {
	if (isBaby()) {
		return (random.nextFloat() - random.nextFloat()) * 0.2f + 1.5f;
	}
	return (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f;
}

int Mob::getAmbientSoundInterval()
{
	return 8 * SharedConstants::TicksPerSecond;
}

void Mob::playAmbientSound()
{
	const char* ambient = getAmbientSound();
	if (ambient != NULL) {
		level->playSound(this, ambient, getSoundVolume(), getVoicePitch());
	}
}

void Mob::baseTick()
{
	oAttackAnim = attackAnim;
	super::baseTick();

	TIMER_PUSH("mobBaseTick");

	if (((ambientSoundTime++ & 15) == 0) && random.nextInt(2000) < ambientSoundTime) {
	    ambientSoundTime = -getAmbientSoundInterval();
	    playAmbientSound();
	}

	if (isAlive() && isInWall()) {
		hurt(NULL, 1);
	}

	//if (fireImmune || level.isOnline) onFire = 0;

	if (isAlive() && isUnderLiquid(Material::water) && !isWaterMob()) {
		airSupply--;
		if (airSupply == -20) {
			airSupply = 0;
			for (int i = 0; i < 8; i++) {
				float xo = random.nextFloat() - random.nextFloat();
				float yo = random.nextFloat() - random.nextFloat();
				float zo = random.nextFloat() - random.nextFloat();
				level->addParticle(PARTICLETYPE(bubble), x + xo, y + yo, z + zo, xd, yd, zd);
			}
			hurt(NULL, 2);
		}
		//onFire = 0;
	} else {
		airSupply = airCapacity;
	}

	oTilt = tilt;
	if (attackTime > 0) attackTime--;
	if (hurtTime > 0) hurtTime--;
	if (invulnerableTime > 0) invulnerableTime--;

	if (health <= 0) {
	    deathTime++;
	    if (deathTime > SharedConstants::TicksPerSecond) {
	        beforeRemove();
	        remove();
	        for (int i = 0; i < 20; i++) {
	            float xa = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
				float ya = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
				float za = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
	            level->addParticle(PARTICLETYPE(explode), x + random.nextFloat() * bbWidth * 2 - bbWidth, y + random.nextFloat() * bbHeight, z + random.nextFloat() * bbWidth * 2 - bbWidth, xa, ya, za);
	        }
	    }
	}

	animStepO = animStep;

	yBodyRotO = yBodyRot;
	yRotO = yRot;
	xRotO = xRot;

	TIMER_POP();

	if (!level->isClientSide) {
		if (autoSendPosRot) {
			if (autoSendPosRot && (std::abs(x - sentX) > .1f || std::abs(y - sentY) > .05f || std::abs(z - sentZ) > .1f || std::abs(sentRotX - xRot) > 1 || std::abs(sentRotY - yRot) > 1)) {
				MoveEntityPacket_PosRot packet(this);
				level->raknetInstance->send(packet);
				sentX = x;
				sentY = y;
				sentZ = z;
				sentRotX = xRot;
				sentRotY = yRot;
			}
		}

		float ddx = std::abs(xd - sentXd);
		float ddy = std::abs(yd - sentYd);
		float ddz = std::abs(zd - sentZd);
		const float max = 0.02f;
		const float diff = ddx*ddx + ddy*ddy + ddz*ddz;
		if (diff > max*max || (diff > 0 && xd == 0 && yd == 0 && zd == 0)) {
			sentXd = xd;
			sentYd = yd;
			sentZd = zd;
			SetEntityMotionPacket packet(this);
			level->raknetInstance->send(packet);
			//LOGI("Motion-packet: %d\n", entityId);
		}
	}
}

void Mob::spawnAnim()
{
	/*
	for (int i = 0; i < 20; i++) {
	float xa = random.nextGaussian() * 0.02;
	float ya = random.nextGaussian() * 0.02;
	float za = random.nextGaussian() * 0.02;
	float dd = 10;
	level.addParticle(PARTICLETYPE(explode), x + random.nextFloat() * bbWidth * 2 - bbWidth - xa * dd, y + random.nextFloat() * bbHeight - ya * dd, z + random.nextFloat() * bbWidth * 2 - bbWidth - za
	* dd, xa, ya, za);
	}
	*/
}

void Mob::lerpTo( float x, float y, float z, float yRot, float xRot, int steps )
{
	//heightOffset = 0;
	lx = x;
	ly = y + heightOffset;
	lz = z;
	lyr = yRot;
	lxr = xRot;

	lSteps = steps;
}

void Mob::superTick()
{
	super::tick();
}

void Mob::tick()
{
	super::tick();

	if (arrowCount > 0) {
		if (removeArrowTime <= 0) {
			removeArrowTime = 20 * 3;
		}
		removeArrowTime--;
		if (removeArrowTime <= 0) {
			arrowCount--;
		}
	}

	if (lSteps > 0) {
		float xt = x + (lx - x) / lSteps;
		float yt = y + (ly - y) / lSteps;
		float zt = z + (lz - z) / lSteps;

		float yrd = lyr - yRot;
		while (yrd < -180)
			yrd += 360;
		while (yrd >= 180)
			yrd -= 360;

		yRot += (yrd) / lSteps;
		xRot += (lxr - xRot) / lSteps;

		lSteps--;
		this->setPos(xt, yt, zt);
		this->setRot(yRot, xRot);
	}

	aiStep();

	updateWalkAnim();

	float xd = x - xo;
	float zd = z - zo;

	float sideDist = (float) Mth::sqrt(xd * xd + zd * zd);

	float yBodyRotT = yBodyRot;

	float walkSpeed = 0;
	oRun = run;
	float tRun = 0;
	if (sideDist <= 0.05f) {
		// animStep = 0;
	} else {
		tRun = 1;
		walkSpeed = sideDist * 3;
		yBodyRotT = Mth::atan2(zd, xd) * Mth::RADDEG - 90;
	}
	if (attackAnim > 0) {
		yBodyRotT = yRot;
	}
	if (!onGround) {
		tRun = 0;
	}
	run = run + (tRun - run) * 0.3f;

	/*
	* float yBodyRotD = yRot-yBodyRot; while (yBodyRotD < -180) yBodyRotD
	* += 360; while (yBodyRotD >= 180) yBodyRotD -= 360; yBodyRot +=
	* yBodyRotD * 0.1f;
	*/

	float yBodyRotD = yBodyRotT - yBodyRot;
	while (yBodyRotD < -180)
		yBodyRotD += 360;
	while (yBodyRotD >= 180)
		yBodyRotD -= 360;
	yBodyRot += yBodyRotD * 0.3f;

	float headDiff = yRot - yBodyRot;
	while (headDiff < -180)
		headDiff += 360;
	while (headDiff >= 180)
		headDiff -= 360;
	bool behind = headDiff < -90 || headDiff >= 90;
	if (headDiff < -75) headDiff = -75;
	if (headDiff >= 75) headDiff = +75;
	yBodyRot = yRot - headDiff;
	if (headDiff * headDiff > 50 * 50) {
		yBodyRot += headDiff * 0.2f;
	}

	if (behind) {
		walkSpeed *= -1;
	}
	while (yRot - yRotO < -180)
		yRotO -= 360;
	while (yRot - yRotO >= 180)
		yRotO += 360;

	while (yBodyRot - yBodyRotO < -180)
		yBodyRotO -= 360;
	while (yBodyRot - yBodyRotO >= 180)
		yBodyRotO += 360;

	while (xRot - xRotO < -180)
		xRotO -= 360;
	while (xRot - xRotO >= 180)
		xRotO += 360;
	animStep += walkSpeed;
}

void Mob::setSize( float w, float h )
{
	super::setSize(w, h);
}

void Mob::heal( int heal )
{
	if (health <= 0) return;
	health += heal;
	if (health > 20) health = 20;
	invulnerableTime = invulnerableDuration / 2;
}

bool Mob::hurt( Entity* source, int dmg )
{
	if (level->isClientSide) return false;
	noActionTime = 0;
	if (health <= 0) return false;

	this->walkAnimSpeed = 1.5f;

	bool sound = true;
	if (invulnerableTime > invulnerableDuration / 2.0f) {
		if (dmg <= lastHurt) return false;
		actuallyHurt(dmg - lastHurt);
		lastHurt = dmg;
		sound = false;
	} else {
		lastHurt = dmg;
		lastHealth = health;
		invulnerableTime = invulnerableDuration;
		actuallyHurt(dmg);
		hurtTime = hurtDuration = 10;
		hurtDuration = 10;
	}

	hurtDir = 0;

	if (sound) {
		level->broadcastEntityEvent(this, EntityEvent::HURT);
		markHurt();
		if (source != NULL) {
			float xd = source->x - x;
			float zd = source->z - z;
			while (xd * xd + zd * zd < 0.0001) {
				xd = (Mth::random() - Mth::random()) * 0.01f;
				zd = (Mth::random() - Mth::random()) * 0.01f;
			}
			hurtDir = (float) (std::atan2(zd, xd) * Mth::RADDEG) - yRot;
			knockback(source, dmg, xd, zd);
		} else {
			hurtDir = (int) (Mth::random() * 2.0f) * 180.0f;
		}
	}

	if (health <= 0) {
		if (sound) level->playSound(this, getDeathSound(), getSoundVolume(), getVoicePitch());
		die(source);
	} else {
		if (sound) level->playSound(this, getHurtSound(), getSoundVolume(), getVoicePitch());
	}

	return true;
}

void Mob::actuallyHurt( int dmg )
{
    if (!bypassArmor) {
	    dmg = getDamageAfterArmorAbsorb(dmg);
    }
	health -= dmg;
}

float Mob::getSoundVolume()
{
	return 1;
}

const char* Mob::getAmbientSound()
{
	return NULL;
}

std::string Mob::getHurtSound()
{
	return "random.hurt";
}

std::string Mob::getDeathSound()
{
	return "random.hurt";
}

void Mob::knockback( Entity* source, int dmg, float xd, float zd )
{
	float dd = Mth::invSqrt(xd * xd + zd * zd);
	float pow = 0.4f;

	this->xd *= 0.5f;
	this->yd *= 0.5f;
	this->zd *= 0.5f;

	this->xd -= xd * dd * pow;
	this->yd += 0.4f;
	this->zd -= zd * dd * pow;

	if (this->yd > 0.4f) this->yd = 0.4f;
}

void Mob::die( Entity* source )
{
	if (deathScore > 0 && source != NULL) source->awardKillScore(this, deathScore);

	if (!level->isClientSide) {
		if (!isBaby()) {
			dropDeathLoot();
		}
		level->broadcastEntityEvent(this, EntityEvent::DEATH);
	}
}

void Mob::dropDeathLoot()
{
	int loot = getDeathLoot();
	if (loot > 0) {
		int count = random.nextInt(3);
		for (int i = 0; i < count; i++)
			spawnAtLocation(loot, 1);
	}
}

int Mob::getDeathLoot()
{
	return 0;
}

void Mob::reset() {
	super::reset();
	this->_init();
}

void Mob::_init() {
	yBodyRot = 0;
	yBodyRotO = 0;
	rotOffs = 0;
	fallTime = 0;
	lastHurt = 0;
	deathTime = 0;
	attackTime = 0;
	tilt = 0;
	oTilt = 0;
	hurtDuration = 0;
	hurtTime = 0;
	hurtDir = 0;
	lookTime = 0;
	noActionTime = 0;
	xxa = 0; yya = 0;
	yRotA = 0;

	health = 10;
	lastHealth = 20;
}

void Mob::causeFallDamage( float distance )
{
	int dmg = (int) ceil((distance - 3));

	if (dmg > 0) {
		level->playSound(this, (dmg > 4)?"damage.fallbig":"damage.fallsmall", 0.75f, 1);

		hurt(NULL, dmg);

		int t = level->getTile(Mth::floor(x), Mth::floor(y - 0.2f - this->heightOffset), Mth::floor(z));
		if (t > 0) {
			const Tile::SoundType* soundType = Tile::tiles[t]->soundType;
			level->playSound(this, soundType->getStepSound(), soundType->getVolume() * 0.5f, soundType->getPitch() * 0.75f);
		}
	}
}

void Mob::travel( float xa, float ya )
{
	if (isInWater()) {
		float yo = y;
		moveRelative(xa, ya, 0.02f);
		move(xd, yd, zd);

		xd *= 0.80f;
		yd *= 0.80f;
		zd *= 0.80f;
		yd -= 0.02f;

		if (horizontalCollision && isFree(xd, yd + 0.6f - y + yo, zd)) {
			yd = 0.3f;
		}
	} else if (isInLava()) {
		float yo = y;
		moveRelative(xa, ya, 0.02f);
		move(xd, yd, zd);
		xd *= 0.50f;
		yd *= 0.50f;
		zd *= 0.50f;
		yd -= 0.02f;

		if (horizontalCollision && isFree(xd, yd + 0.6f - y + yo, zd)) {
			yd = 0.3f;
		}
	} else {
		float friction = 0.91f;
		if (onGround) {
			friction = 0.6f * 0.91f;
			int t = level->getTile(Mth::floor(x), Mth::floor(bb.y0 - 0.5f), Mth::floor(z));
			if (t > 0) {
				friction = Tile::tiles[t]->friction * 0.91f;
			}
		}

		float friction2 = (0.6f * 0.6f * 0.91f * 0.91f * 0.6f * 0.91f) / (friction * friction * friction);
		moveRelative(xa, ya, (onGround ? walkingSpeed * friction2 : flyingSpeed));

		friction = 0.91f;
		if (onGround) {
			friction = 0.6f * 0.91f;
			int t = level->getTile(Mth::floor(x), Mth::floor(bb.y0 - 0.5f), Mth::floor(z));
			if (t > 0) {
				friction = Tile::tiles[t]->friction * 0.91f;
			}
		}
		//@todo: make it easier to climb ladders
		if (onLadder()) {
			this->fallDistance = 0;
			if (yd < -0.15) yd = -0.15f;
			if (isSneaking() && yd < 0) yd = 0;
		}

		move(xd, yd, zd);

		//@todo: make it easier to climb ladders
		if (horizontalCollision && onLadder()) {
			yd = 0.2f;
		}

		yd -= 0.08f;
		yd *= 0.98f;
		xd *= friction;
		zd *= friction;
	}

}

void Mob::updateWalkAnim()
{
	walkAnimSpeedO = walkAnimSpeed;
	float xxd = x - xo;
	float zzd = z - zo;
	float wst = Mth::sqrt(xxd * xxd + zzd * zzd) * 4;
	if (wst > 1) wst = 1;
	walkAnimSpeed += (wst - walkAnimSpeed) * 0.4f;
	walkAnimPos += walkAnimSpeed;
}

bool Mob::onLadder()
{
	int xt = Mth::floor(x);
	int yt = Mth::floor(bb.y0);
	int zt = Mth::floor(z);
	return level->getTile(xt, yt, zt) == Tile::ladder->id || level->getTile(xt, yt + 1, zt) == Tile::ladder->id;
}

bool Mob::isShootable()
{
	return true;
}

void Mob::addAdditonalSaveData( CompoundTag* entityTag )
{
	entityTag->putShort("Health", (short) health);
	entityTag->putShort("HurtTime", (short) hurtTime);
	entityTag->putShort("DeathTime", (short) deathTime);
	entityTag->putShort("AttackTime", (short) attackTime);

	//if (isPlayer()) LOGI("Saving %d, %d, %d, %d\n", health, hurtTime, deathTime, attackTime);
}

void Mob::readAdditionalSaveData( CompoundTag* tag )
{
	health = tag->getShort("Health");
	hurtTime = tag->getShort("HurtTime");
	deathTime = tag->getShort("DeathTime");
	attackTime = tag->getShort("AttackTime");

	//if (isPlayer()) LOGI("Reading %d, %d, %d, %d\n", health, hurtTime, deathTime, attackTime);
}

void Mob::animateHurt()
{
	hurtTime = hurtDuration = 10;
	hurtDir = 0;
}

bool Mob::isAlive()
{
	return !removed && health > 0;
}

bool Mob::isWaterMob()
{
	return false;
}

void Mob::aiStep()
{
	//@todo? 30 lines of code here in java version

	TIMER_PUSH("ai");
	if (isImmobile()) {
		jumping = false;
		xxa = 0;
		yya = 0;
		yRotA = 0;
	} else {
		if (!interpolateOnly()) {
			if (useNewAi()) {
				TIMER_PUSH("newAi");
				newServerAiStep();
				TIMER_POP();
			}
			else {
				TIMER_PUSH("oldAi");
				updateAi();
				TIMER_POP();
			}
		}
	}
	TIMER_POP_PUSH("move");

	bool inWater = isInWater();
	bool inLava = isInLava();

	if (jumping) {
		if (inWater) {
			yd += 0.04f;
		} else if (inLava) {
			yd += 0.04f;
		} else if (onGround) {
			jumpFromGround();
		}
	}

	xxa *= 0.98f;
	yya *= 0.98f;
	yRotA *= 0.9f;

	float normalSpeed = walkingSpeed;
	float normalAirControllSpeed = flyingSpeed;
	walkingSpeed *= getWalkingSpeedModifier();
	flyingSpeed  *= getWalkingSpeedModifier();
	travel(xxa, yya);
	walkingSpeed = normalSpeed;
	flyingSpeed = normalAirControllSpeed;

	TIMER_POP_PUSH("push");
	Player* player = NULL;
	if(isPlayer()) {
		player = (Player*)this;
	}
	if(player == NULL || !player->isSleeping())  {
		EntityList& entities = level->getEntities(this, this->bb.grow(0.2f, 0, 0.2f));
		for (unsigned int i = 0; i < entities.size(); i++) {
			Entity* e = entities[i];
			if(e->isPlayer()) {
				Player* p = (Player*)e;
				if(p->isSleeping())
					continue;
			}
			if (e->isPushable()) e->push(this);
		}
	}
	TIMER_POP();
}

/*protected*/
void Mob::updateAi() {
    noActionTime++;

	Player* player = level->getNearestPlayer(this, -1);
	checkDespawn(player);

    xxa = 0;
    yya = 0;

    float lookDistance = 8;
    if (random.nextFloat() < 0.02f) {
        player = level->getNearestPlayer(this, lookDistance);
        if (player != NULL) {
            lookingAtId = player->entityId;
            lookTime = 10 + random.nextInt(20);
        } else {
            yRotA = (random.nextFloat() - 0.5f) * 20;
        }
    }

    if (lookingAtId != 0) {
		Entity* lookingAt = level->getEntity(lookingAtId);
		lookingAtId = 0;
		if (lookingAt != NULL) {
			lookAt(lookingAt, 10, (float)getMaxHeadXRot());
			if (lookTime-- <= 0 || lookingAt->removed || lookingAt->distanceToSqr(this) > lookDistance * lookDistance) {
	            lookingAtId = 0;
		    } else {
				lookingAtId = lookingAt->entityId;
			}
		}
    } else {
        if (random.nextFloat() < 0.05f) {
            yRotA = (random.nextFloat() - 0.5f) * 20;
        }
        yRot += yRotA;
        xRot = defaultLookAngle;
    }

    bool inWater = isInWater();
    bool inLava = isInLava();
    if (inWater || inLava) jumping = random.nextFloat() < 0.8f;
}

void Mob::newServerAiStep() {
	noActionTime++;

	TIMER_PUSH("checkDespawn");
	checkDespawn();
	jumping = false;

	TIMER_POP_PUSH("sensing");
	if (sensing) sensing->tick();

	if (random.nextInt(5) == 0) {
		TIMER_POP_PUSH("targetSelector");
		//LOGI("tg1\n");
		if (targetSelector) targetSelector->tick();
		//LOGI("tg2\n");

		TIMER_POP_PUSH("goalSelector");
		//LOGI("gs1\n");
		if (goalSelector) goalSelector->tick();
		//LOGI("gs2\n");
	}

	TIMER_POP_PUSH("navigation");
	if (navigation) navigation->tick();

	//TIMER_POP_PUSH("mob tick");
	//serverAiMobStep();

	TIMER_POP_PUSH("controls");
	if (moveControl) moveControl->tick(); //@todo; we shouldn't be here if not
	if (jumpControl) jumpControl->tick();

	TIMER_POP();
}

bool Mob::isImmobile()
{
	return health <= 0;
}

void Mob::jumpFromGround()
{
	yd = 0.42f;
}

bool Mob::removeWhenFarAway()
{
	return true;
}

int Mob::getMaxHeadXRot()
{
	return 10;
}

void Mob::lookAt( Entity* e, float yMax, float xMax )
{
	float xd = e->x - x;
	float yd;
	float zd = e->z - z;

	if (false) { //if (e->isMob()) { //@todo?
		Mob* mob = (Mob*) e;
		yd = (y + getHeadHeight()) - (mob->y + mob->getHeadHeight());
	} else {
		yd = (e->bb.y0 + e->bb.y1) / 2 - (y + getHeadHeight());
	}

	float sd = Mth::sqrt(xd * xd + zd * zd);

	float yRotD = (float) (atan2(zd, xd) * 180 / Mth::PI) - 90;
	float xRotD = (float) (atan2(yd, sd) * 180 / Mth::PI);
	xRot = -rotlerp(xRot, xRotD, xMax);
	yRot = rotlerp(yRot, yRotD, yMax);
}

bool Mob::isLookingAtAnEntity()
{
	return lookingAtId != 0;
}

//Entity* Mob::getLookingAt()
//{
//	return lookingAt;
//}

float Mob::rotlerp( float a, float b, float max )
{
	float diff = b - a;
	while (diff < -180)
		diff += 360;
	while (diff >= 180)
		diff -= 360;
	if (diff > max) {
		diff = max;
	}
	if (diff < -max) {
		diff = -max;
	}
	return a + diff;
}

void Mob::beforeRemove()
{
}

bool Mob::canSpawn()
{
	return level->isUnobstructed(bb) && level->getCubes(this, bb).size() == 0 && !level->containsAnyLiquid(bb);
}

void Mob::outOfWorld()
{
	int oldHealth = health;
	hurt(NULL, 4);
	if (health >= oldHealth)
		actuallyHurt(4);
}

float Mob::getAttackAnim( float a )
{
	float diff = attackAnim - oAttackAnim;
	if (diff < 0) diff += 1;
	return oAttackAnim + diff * a;
}

Vec3 Mob::getPos( float a )
{
	if (a == 1) {
		return Vec3(x, y, z);
	}
	float x = xo + (this->x - xo) * a;
	float y = yo + (this->y - yo) * a;
	float z = zo + (this->z - zo) * a;

	return Vec3(x, y, z);
}

Vec3 Mob::getLookAngle()
{
	return getViewVector(1);
}

Vec3 Mob::getViewVector( float a )
{
	if (a == 1) {
		float yCos = (float) Mth::cos(-yRot * Mth::DEGRAD - Mth::PI);
		float ySin = (float) Mth::sin(-yRot * Mth::DEGRAD - Mth::PI);
		float xCos = (float) -Mth::cos(-xRot * Mth::DEGRAD);
		float xSin = (float) Mth::sin(-xRot * Mth::DEGRAD);

		return Vec3(ySin * xCos, xSin, yCos * xCos); // newTemp
	}
	float xRot = xRotO + (this->xRot - xRotO) * a;
	float yRot = yRotO + (this->yRot - yRotO) * a;

	float yCos = (float) Mth::cos(-yRot * Mth::DEGRAD - Mth::PI);
	float ySin = (float) Mth::sin(-yRot * Mth::DEGRAD - Mth::PI);
	float xCos = (float) -Mth::cos(-xRot * Mth::DEGRAD);
	float xSin = (float) Mth::sin(-xRot * Mth::DEGRAD);

	return Vec3(ySin * xCos, xSin, yCos * xCos); // newTemp
}

HitResult Mob::pick( float range, float a )
{
	Vec3 from = getPos(a);
	return level->clip(from, from + getViewVector(a) * range);
}

int Mob::getMaxSpawnClusterSize()
{
	return 4;
}

bool Mob::interpolateOnly()
{
	return level->isClientSide;
}

//ItemInstance getCarriedItem() {
//    return NULL;
//}

void Mob::handleEntityEvent(char id) {
    if (id == EntityEvent::HURT) {
        this->walkAnimSpeed = 1.5f;

        invulnerableTime = invulnerableDuration;
        hurtTime = hurtDuration = 10;
        hurtDir = 0;

        level->playSound(this, getHurtSound(), getSoundVolume(), getVoicePitch());
        hurt(NULL, 0);
    } else if (id == EntityEvent::DEATH) {
        level->playSound(this, getDeathSound(), getSoundVolume(), getVoicePitch());
        health = 0;
        die(NULL);
    } else {
        super::handleEntityEvent(id);
    }
}

SynchedEntityData* Mob::getEntityData() {
	return &entityData;
}

const SynchedEntityData* Mob::getEntityData() const {
	return &entityData;
}

MoveControl* Mob::getMoveControl() {
	return moveControl;
}

JumpControl* Mob::getJumpControl() {
	return jumpControl;
}

void Mob::setYya( float yya )
{
	this->yya = yya;
}

float Mob::getSpeed() {
	return speed;
}

void Mob::setSpeed( float speed ) {
	this->speed = speed;
	this->yya = speed;
}

void Mob::setJumping(bool jump) {
	jumping = jump;
}

bool Mob::useNewAi() {
	return false;
}

void Mob::checkDespawn() {
	checkDespawn(level->getNearestPlayer(this, -1));
}

void Mob::checkDespawn(Mob* nearestBlocking) {
	if (nearestBlocking != NULL) {
		const bool removeIfFar = removeWhenFarAway();
		float xd = nearestBlocking->x - x;
		float yd = nearestBlocking->y - y;
		float zd = nearestBlocking->z - z;
		float sd = xd * xd + yd * yd + zd * zd;

		if (removeIfFar && sd > 96 * 96) {
			//LOGI("removed some angry mob more than 10 meters away!\n");
			remove();
		}

		if (noActionTime > 30 * SharedConstants::TicksPerSecond && random.nextInt(800) == 0 && removeIfFar && sd > 32 * 32)
			remove();
		else
			noActionTime = 0;
	}
}
bool Mob::getSharedFlag(int flag) {
	return entityData.getFlag<SharedFlagsInformation::SharedFlagsInformationType> (SharedFlagsInformation::DATA_SHARED_FLAGS_ID, flag);
}
void Mob::setSharedFlag(int flag, bool value) {
	if(value) {
		entityData.setFlag<SharedFlagsInformation::SharedFlagsInformationType>(SharedFlagsInformation::DATA_SHARED_FLAGS_ID, flag);
	}
	else {
		entityData.clearFlag<SharedFlagsInformation::SharedFlagsInformationType>(SharedFlagsInformation::DATA_SHARED_FLAGS_ID, flag);
	}
}

void Mob::updateAttackAnim() {
	if (swinging) {
		if (++swingTime >= SWING_DURATION) {
			swingTime = 0;
			swinging = false;
			//LOGI("Swinging! %d/%d (%f)\n", swingTime, SWING_DURATION, attackAnim);
		}
	} else {
		swingTime = 0;
	}

	attackAnim = swingTime / (float) SWING_DURATION;
}

void Mob::swing() {
	if (!swinging || swingTime >= 3 || swingTime < 0) {
		swingTime = -1;
		swinging = true;
	}
}

bool Mob::isSneaking() {
    return getSharedFlag(SharedFlagsInformation::FLAG_SNEAKING);
}

bool Mob::isSleeping() {
    return false;
}

void Mob::setSneaking(bool value) {
    setSharedFlag(SharedFlagsInformation::FLAG_SNEAKING, value);
}

//
// Armor
//
int Mob::getDamageAfterArmorAbsorb(int damage) {
    int absorb = 25 - getArmorValue();
    int v = (damage) * absorb + dmgSpill;
    hurtArmor(damage);
    damage = v / 25;
    dmgSpill = v % 25;
    return damage;
}

static std::vector<ItemInstance*> getEquipmentSlots() {
	return std::vector<ItemInstance*>();
}

int Mob::getArmorValue() {
	int val = 0;
    /*
	const std::vector<ItemInstance*> items = getEquipmentSlots();
	for (unsigned int i = 0; i < items.size(); ++i) {
		ItemInstance* item = items[i];
        if (item != NULL && ItemInstance::isArmorItem(item)) {
            int baseProtection = ((ArmorItem*) item->getItem())->defense;
            val += baseProtection;
        }
    }
    */
    return val;
}

void Mob::hurtArmor(int damage) {}
