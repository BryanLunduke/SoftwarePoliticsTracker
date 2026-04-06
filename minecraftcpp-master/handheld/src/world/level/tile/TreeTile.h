#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"
#include "LeafTile.h"

class TreeTile: public Tile
{
public:
	static const int NORMAL_TRUNK = 0;
    static const int DARK_TRUNK   = 1;
    static const int BIRCH_TRUNK  = 2;

	TreeTile(int id)
	:	Tile(id, Material::wood)
	{
        tex = 20;
    }

    int getResourceCount(Random* random) {
        return 1;
    }

    int getResource(int data, Random* random) {
        return Tile::treeTrunk->id;
    }

    void onRemove(Level* level, int x, int y, int z) {
        int r = LeafTile::REQUIRED_WOOD_RANGE;
        int r2 = r + 1;

        if (level->hasChunksAt(x - r2, y - r2, z - r2, x + r2, y + r2, z + r2)) {
            for (int xo = -r; xo <= r; xo++)
            for (int yo = -r; yo <= r; yo++)
            for (int zo = -r; zo <= r; zo++) {
                int t = level->getTile(x + xo, y + yo, z + zo);
                if (t == Tile::leaves->id) {
                    int currentData = level->getData(x + xo, y + yo, z + zo);
                    if ((currentData & LeafTile::UPDATE_LEAF_BIT) == 0) {
                        level->setDataNoUpdate(x + xo, y + yo, z + zo, currentData | LeafTile::UPDATE_LEAF_BIT);
                    }
                }
            }
        }
    }

    int getTexture(int face, int data) {
        if (face == 1) return 21;
        if (face == 0) return 21;
        if (data == DARK_TRUNK)  return 4 + 7 * 16;
        if (data == BIRCH_TRUNK) return 5 + 7 * 16;
        return 20;
    }

protected:
    int getSpawnResourcesAuxValue(int data) {
        return data;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TreeTile_H__*/
