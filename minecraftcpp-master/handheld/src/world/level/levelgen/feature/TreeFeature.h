#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TreeFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TreeFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"

#include "../../../../util/Random.h"
#include "../../Level.h"

#include "../../tile/TreeTile.h"

class TreeFeature: public Feature
{
	typedef Feature super;
public:
	TreeFeature(bool doUpdate, int trunkType = TreeTile::NORMAL_TRUNK)
	:	super(doUpdate),
		trunkType(trunkType)
	{
	}

	bool place(Level* level, Random* random, int x, int y, int z) {
        int treeHeight = random->nextInt(3) + 4;

        bool free = true;
        if (y < 1 || y + treeHeight + 1 > Level::DEPTH) return false;

        for (int yy = y; yy <= y + 1 + treeHeight; yy++) {
            int r = 1;
            if (yy == y) r = 0;
            if (yy >= y + 1 + treeHeight - 2) r = 2;
            for (int xx = x - r; xx <= x + r && free; xx++) {
                for (int zz = z - r; zz <= z + r && free; zz++) {
                    if (yy >= 0 && yy < Level::DEPTH) {
                        int tt = level->getTile(xx, yy, zz);
                        if (tt != 0 && tt != ((Tile*)Tile::leaves)->id) free = false;
                    } else {
                        free = false;
                    }
                }
            }
        }

        if (!free) return false;

        int belowTile = level->getTile(x, y - 1, z);
        if ((belowTile != ((Tile*)Tile::grass)->id && belowTile != Tile::dirt->id) || y >= Level::DEPTH - treeHeight - 1) return false;

        placeBlock(level, x, y - 1, z, Tile::dirt->id);

        for (int yy = y - 3 + treeHeight; yy <= y + treeHeight; yy++) {
            int yo = yy - (y + treeHeight);
            int offs = 1 - yo / 2;
            for (int xx = x - offs; xx <= x + offs; xx++) {
                int xo = xx - (x);
                for (int zz = z - offs; zz <= z + offs; zz++) {
                    int zo = zz - (z);
                    if (std::abs(xo) == offs && std::abs(zo) == offs && (random->nextInt(2) == 0 || yo == 0)) continue;
                    if (!Tile::solid[level->getTile(xx, yy, zz)]) placeBlock(level, xx, yy, zz, ((Tile*)Tile::leaves)->id);
                }
            }
        }
        for (int hh = 0; hh < treeHeight; hh++) {
            int t = level->getTile(x, y + hh, z);
            if (t == 0 || t == ((Tile*)Tile::leaves)->id) placeBlock(level, x, y + hh, z, Tile::treeTrunk->id, trunkType);
        }
        return true;
    }
private:
	int trunkType;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TreeFeature_H__*/
