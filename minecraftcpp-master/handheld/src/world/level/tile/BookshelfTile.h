#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__BookshelfTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__BookshelfTile_H__

//package net.minecraft.world.level.tile;

#include "../../item/Item.h"
#include "../material/Material.h"

class BookshelfTile: public Tile
{
	typedef Tile super;
public:
    BookshelfTile(int id, int tex)
    :   super(id, tex, Material::wood)
    {
    }

    int getTexture(int face) {
        if (face <= 1) return 4;
        return tex;
    }

    int getResourceCount(Random* random) {
        return 3;
    }

    //@Override
    int getResource(int data, Random* random/*, int playerBonusLevel*/) {
        return Item::book->id;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__BookshelfTile_H__*/
