#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__

//package net.minecraft.world.level.tile;

#include "../material/Material.h"

#include "Tile.h"

class DirtTile: public Tile
{
public:
    /*protected*/
	DirtTile(int id, int tex)
	:	Tile(id, tex, Material::dirt)
    {
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__DirtTile_H__*/
