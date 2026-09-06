#ifndef NET_MINECRAFT_WORLD_ENTITY__Mob_H__
#define NET_MINECRAFT_WORLD_ENTITY__Mob_H__

//package net.minecraft.world.entity;

#include <string>
#include "Entity.h"
#include "EntityTypes.h"
#include "SynchedEntityData.h"


class CompoundTag;

class Level;
class CompundTag;

class MoveControl;
class JumpControl;
class PathNavigation;
class GoalSelector;
class Sensing;

class Mob: public Entity
{
	typedef Entity super;
public:
    static const int ATTACK_DURATION = 5;
	static const int SWING_DURATION = 8;

    Mob(Level* level);
	virtual ~Mob();

	void _init();
	virtual void reset();

	virtual void knockback(Entity* source, int dmg, float xd, float zd);
	virtual void die(Entity* source);

    virtual bool canSee(Entity* target);

	virtual bool onLadder();

	virtual void spawnAnim();
    virtual std::string getTexture();

	virtual bool isAlive();
    virtual bool isPickable();
    virtual bool isPushable();
	virtual bool isShootable();

	MoveControl* getMoveControl();
	JumpControl* getJumpControl();

	virtual bool isSleeping();
	virtual bool isWaterMob();

    virtual bool isSneaking();
    virtual void setSneaking(bool value);

    virtual float getHeadHeight();

	virtual float getVoicePitch();

	virtual void playAmbientSound();
    virtual int getAmbientSoundInterval();
	virtual int getItemInHandIcon(ItemInstance* item, int layer) {
		return item->getIcon();	
	}
    virtual void lerpTo(float x, float y, float z, float yRot, float xRot, int steps);

	void	setYya( float yya );
	float	getSpeed();
	void	setSpeed(float speed);
	void	setJumping(bool jump);

	virtual void tick();
	virtual void baseTick();
    virtual void superTick();

    virtual void heal(int heal);
    virtual int  getMaxHealth() { return 10; }
	virtual bool hurt(Entity* source, int dmg);
    virtual void actuallyHurt(int dmg);
    virtual void animateHurt();
    virtual int getArmorValue();

	virtual HitResult pick(float range, float a);

	virtual void travel(float xa, float ya);
	virtual void updateWalkAnim();

	virtual void aiStep();

	virtual SynchedEntityData* getEntityData();
	virtual const SynchedEntityData* getEntityData() const;

	virtual void addAdditonalSaveData(CompoundTag* entityTag);
	virtual void readAdditionalSaveData(CompoundTag* tag);

	virtual void lookAt(Entity* e, float yMax, float xMax);
	virtual bool isLookingAtAnEntity();
	//virtual Entity* getLookingAt();

	virtual void beforeRemove();

	virtual bool canSpawn();

	virtual float getAttackAnim(float a);

	virtual Vec3 getPos(float a);
	virtual Vec3 getLookAngle();
	virtual Vec3 getViewVector(float a);

	virtual int getMaxSpawnClusterSize();

	virtual bool isMob() { return true; }
	virtual bool isBaby() { return false; }

	virtual void handleEntityEvent(char id);
	virtual ItemInstance* getCarriedItem() {return NULL;}
	virtual int getUseItemDuration() {return 0;}
	virtual void swing();
protected:
	virtual void causeFallDamage(float distance);

	virtual void outOfWorld();
	virtual bool removeWhenFarAway();

	virtual int getDeathLoot();
    virtual void dropDeathLoot();

    virtual bool isImmobile();

	virtual void jumpFromGround();

    virtual void updateAi();
	virtual void newServerAiStep();
	
	virtual void setSize(float w, float h);

	virtual int getMaxHeadXRot();

	virtual float getSoundVolume();
	virtual const char* getAmbientSound();
	virtual std::string getHurtSound();
	virtual std::string getDeathSound();

	virtual float getWalkingSpeedModifier();

    virtual int getDamageAfterArmorAbsorb(int damage);
    virtual void hurtArmor(int damage);

	bool interpolateOnly();
	virtual bool useNewAi();
	bool getSharedFlag(int flag);
	void setSharedFlag(int flag, bool value);
	void checkDespawn(Mob* nearestBlocking);
	void checkDespawn();
	void updateAttackAnim();
private:
	float rotlerp(float a, float b, float max);
public:
	int invulnerableDuration;

	float timeOffs;

	float rotA;
	float yBodyRot, yBodyRotO;
	//bool interpolateOnly;

	float oAttackAnim, attackAnim;

	int health;
	int lastHealth;

	int hurtTime;
	int hurtDuration;
	float hurtDir;

	int deathTime;
	int attackTime;
	float oTilt, tilt;

	int lookTime;
	float fallTime;

	float walkAnimSpeedO;
	float walkAnimSpeed;
	float walkAnimPos;

    Vec3 aimDirection;
	int arrowCount;
	int removeArrowTime;

	Random random;
	Sensing* sensing;

	//@note: This is a temporary fix for DamageSource that bypasses armor
	//       or creating two functions, that does virtually the same
	//       except one bypasses armor. It's enough virtual calls and chains
	//       of super-calls for me to think this is safer.
	bool bypassArmor;
protected:
	SynchedEntityData entityData;
	bool swinging;
	int swingTime;
	int noActionTime;
	float xxa, yya, yRotA;
	float defaultLookAngle;

	float runSpeed;
	float walkingSpeed;
	float flyingSpeed;

	std::string textureName;
	std::string modelName;
	int deathScore;
	float oRun, run;
	float animStep, animStepO;
	float rotOffs;
	float bobStrength;
	float renderOffset;

	int lSteps;
	float lx, ly, lz, lyr, lxr;
	int lastHurt;
    int dmgSpill;

	float sentX, sentY, sentZ, sentRotX, sentRotY;
	float sentXd, sentYd, sentZd;

	//bool hasHair;
	bool allowAlpha;
	bool jumping;
	bool autoSendPosRot;

	MoveControl* moveControl;
	JumpControl* jumpControl;
	PathNavigation* navigation;
	GoalSelector* goalSelector;
	GoalSelector* targetSelector;
private:
	int lookingAtId;
	int ambientSoundTime;

	float speed;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__Mob_H__*/
