#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__StoneTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__StoneTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"

#include "Tile.h"

class StoneTile: public Tile
{
public:
    StoneTile(int id, int tex)
	:	Tile(id, tex, Material::stone)
	{
    }

    int getResource(int data, Random* random) {
        return Tile::stoneBrick->id;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__StoneTile_H__*/
