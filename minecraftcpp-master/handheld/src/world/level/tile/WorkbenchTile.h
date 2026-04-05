#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__WorkbenchTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__WorkbenchTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/player/Player.h"

class WorkbenchTile: public Tile
{
	typedef Tile super;
public:
    WorkbenchTile(int id)
	:	super(id, Material::wood)
	{
        tex = 11 + 16 * 3;
    }

    int getTexture(int face) {
        if (face == 1) return tex - 16;
        if (face == 0) return Tile::wood->getTexture(0);
        if (face == 2 || face == 4) return tex + 1;
        return tex;
    }

	bool use(Level* level, int x, int y, int z, Player* player) {
		player->startCrafting(x, y, z, Recipe::SIZE_3X3);
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__WorkbenchTile_H__*/
