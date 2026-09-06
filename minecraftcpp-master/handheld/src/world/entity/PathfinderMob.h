#ifndef NET_MINECRAFT_WORLD_ENTITY__PathfinderMob_H__
#define NET_MINECRAFT_WORLD_ENTITY__PathfinderMob_H__

//package net.minecraft.world.entity;

#include "Mob.h"
#include "ai/PathNavigation.h"
#include "../level/pathfinder/Path.h"

class Level;
class Sensing;

typedef struct AIData {
	AIData():
	target(NULL)
	{}

	Mob* target;
} AIData;

class PathfinderMob: public Mob
{
    typedef Mob super;
    static const int MAX_TURN = 30;
public:
    PathfinderMob(Level* level);
	~PathfinderMob();

    bool canSpawn();

    bool isPathFinding();
    void setPath(Path& path);

    virtual Entity* getAttackTarget();
    virtual void	setAttackTarget(Entity* attacker);

	virtual float	getWalkTargetValue(int x, int y, int z);
	int getNoActionTime();

	PathNavigation* getNavigation();
protected:
	virtual Entity* findAttackTarget();

    virtual void checkHurtTarget(Entity* target, float d);
    virtual void checkCantSeeTarget(Entity* target, float d);

    virtual float getWalkingSpeedModifier();

	virtual bool shouldHoldGround();

	void updateAi();

	virtual void findRandomStrollLocation();
	int attackTargetId;
    bool holdGround;
    int fleeTime;
public:
	static const int CAN_OPEN_DOORS = 1;
	static const int AVOID_WATER    = 2;
	//int pathfinderMask;
private:
    Path path;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__PathfinderMob_H__*/
