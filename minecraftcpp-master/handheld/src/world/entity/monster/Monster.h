#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Monster_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Monster_H__

//package net.minecraft.world.entity->monster;

#include "../PathfinderMob.h"

class Level;
class Entity;
//class DamageSource;
class CompoundTag;

class Monster: public PathfinderMob//implements Enemy
{
	typedef PathfinderMob super;
public:
    Monster(Level* level);

	void aiStep();
    void tick();

    //bool hurt(DamageSource* source, int dmg) {
	bool hurt(Entity* sourceEntity, int dmg);

    /*@Override*/
    bool canSpawn();

	int getCreatureBaseType() const;

	bool doHurtTarget(Entity* target);
	void setTarget(Mob* mob);
	Mob* getTarget();

	Mob* getLastHurtByMob();
	void setLastHurtByMob(Mob* mob);
	virtual int getAttackDamage(Entity* target);
protected:
	Entity* findAttackTarget();

	bool isDarkEnoughToSpawn();
    /**
     * Performs hurt action, returns if successful
     * 
     * @param target
     * @return
     */
    void checkHurtTarget(Entity* target, float distance);

    float getWalkTargetValue(int x, int y, int z);
	virtual int getAttackTime();
	int attackDamage;
	int targetId;

	int lastHurtByMobId;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Monster_H__*/
