#ifndef NET_MINECRAFT_WORLD_LEVEL__Explosion_H__
#define NET_MINECRAFT_WORLD_LEVEL__Explosion_H__

//package net.minecraft.world.level;

#include <set>

#include "TilePos.h"
#include "../../util/Random.h"

class Level;
class Entity;

typedef std::set<TilePos> TilePosSet;

class Explosion
{
public:
    Explosion(Level* level, Entity* source, float x, float y, float z, float r);

    void explode();
    void finalizeExplosion();

	float x, y, z;
	float r;

	TilePosSet toBlow;

	bool fire;
	Entity* source;

private:
	Random random;
	Level* level;

};

#endif /*NET_MINECRAFT_WORLD_LEVEL__Explosion_H__*/
