#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__GravelTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__GravelTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"

#include "HeavyTile.h"

class GravelTile: public HeavyTile
{
    typedef HeavyTile super;
public:
    GravelTile(int type, int tex)
	:	super(type, tex)
	{
	}

    int getResource(int data, Random* random) {
        if (random->nextInt(10) == 0) return Item::flint->id;
        return id;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__GravelTile_H__*/
