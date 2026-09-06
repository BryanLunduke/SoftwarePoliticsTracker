#ifndef NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Throwable_H__
#define NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Throwable_H__

//package net.minecraft.world.entity->projectile;

class Level;
class Mob;
class Player;
class CompoundTag;

#include "../Entity.h"

class HitResult;

/*abstract*/
class Throwable: public Entity
{
	typedef Entity super;
public:
    Throwable(Level* level);
	Throwable(Level* level, Mob* mob);
	Throwable(Level* level, float x, float y, float z);

    void shoot(const Vec3& v, float pow, float uncertainty);
    void shoot(float xd, float yd, float zd, float pow, float uncertainty);

	void tick();
    void lerpMotion(float xd, float yd, float zd);

    void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

	bool shouldRenderAtSqrDistance(float distance);

	float getShadowHeightOffs();

    int getAuxData();
protected:
	virtual float getThrowPower();
	virtual float getThrowUpAngleOffset();

	virtual float getGravity();

	virtual void onHit(const HitResult& res) = 0;

private:
	void _init();

public:
	int shakeTime;
protected:
	bool inGround;
	int ownerId;
private:
	int life;
	int flightTime;
	int xTile;
	int yTile;
	int zTile;
	int lastTile;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Throwable_H__*/
