#ifndef NET_MINECRAFT_WORLD_PHYS__HitResult_H__
#define NET_MINECRAFT_WORLD_PHYS__HitResult_H__

//package net.minecraft.world.phys;

#include "Vec3.h"
class Entity;

enum HitResultType {
    TILE,
	ENTITY,
	NO_HIT
};

class HitResult
{
public:
	HitResult();
    HitResult(int x, int y, int z, int f, const Vec3& pos);
    HitResult(Entity* entity);
	HitResult(const HitResult& hr);

	float distanceTo(Entity* entity) const ;

	bool isHit() const { return type != NO_HIT; }

    HitResultType type;
    int x, y, z, f;
    Vec3 pos;
    Entity* entity;
	bool indirectHit; // when targeting the block we're standing on, without actually looking at it
};

#endif /*NET_MINECRAFT_WORLD_PHYS__HitResult_H__*/
