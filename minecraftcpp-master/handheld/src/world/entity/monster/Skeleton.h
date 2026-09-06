#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Skeleton_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Skeleton_H__

//package net.minecraft.world.entity->monster;

#include "Monster.h"
#include <string>
#include "../../item/ItemInstance.h"

class Level;
class Entity;

class Skeleton: public Monster
{
    typedef Monster super;
public:
    Skeleton(Level* level);

    /*@Override*/
    int getMaxHealth();

    void aiStep();

    int getDeathLoot();

    ItemInstance* getCarriedItem();

	virtual int getEntityTypeId() const;
protected:
    const char* getAmbientSound();
    std::string getHurtSound();
    std::string getDeathSound();

    void checkHurtTarget(Entity* target, float d);
    /*@Override*/
    void dropDeathLoot(/*bool wasKilledByPlayer, int playerBonusLevel*/);
	virtual int getUseDuration();
private:
	ItemInstance bow;
	int fireCheckTick;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Skeleton_H__*/
