#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Spider_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Spider_H__

//package net.minecraft.world.entity->monster;

#include "Monster.h"
#include <string>

class Level;
class Entity;

class Spider: public Monster
{
    typedef Monster super;
	typedef SynchedEntityData::TypeChar DataFlagIdType;
    static const int DATA_FLAGS_ID = 16;
public:
    Spider(Level* level);

    /*@Override*/
    void aiStep();
    /*@Override*/
    void tick();

    /*@Override*/
    int getMaxHealth();

    /**
     * The the spiders act as if they're always on a ladder, which enables them
     * to climb walls.
     */
    /*@Override*/ //@todo
    bool onLadder();

    /*@Override*/
    void makeStuckInWeb();

    float getModelScale();

	bool isClimbing();
	void setClimbing(bool value);

	virtual int getEntityTypeId() const;

protected:
    /*@Override*/
    bool makeStepSound();

    Entity* findAttackTarget();

    const char* getAmbientSound();
    std::string getHurtSound();
    std::string getDeathSound();

    void checkHurtTarget(Entity* target, float d);
	int fireCheckTick;
    int getDeathLoot();
    /*@Override*/
    //void dropDeathLoot();
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Spider_H__*/
