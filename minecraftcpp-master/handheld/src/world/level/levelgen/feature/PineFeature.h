#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__PineFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__PineFeature_H__

//package net.minecraft.world.level->levelgen.feature;

#include "Feature.h"
#include "../../../../util/Random.h"
#include "../../Level.h"
#include "../../tile/LeafTile.h"
#include "../../tile/Tile.h"
#include "../../tile/TreeTile.h"

class PineFeature: public Feature
{
	typedef Feature super;
public:
	PineFeature(bool doUpdate = false)
	:	super(doUpdate)
	{
	}

    bool place(Level* level, Random* random, int x, int y, int z) {

        // pines can be quite tall
        int treeHeight = random->nextInt(5) + 7;
        int trunkHeight = treeHeight - random->nextInt(2) - 3;
        int topHeight = treeHeight - trunkHeight;
        int topRadius = 1 + random->nextInt(topHeight + 1);

        bool free = true;
        // may not be outside of y boundaries
        if (y < 1 || y + treeHeight + 1 > Level::DEPTH) {
            return false;
        }

        // make sure there is enough space
        for (int yy = y; yy <= y + 1 + treeHeight && free; yy++) {

            int r = 1;
            if ((yy - y) < trunkHeight) {
                r = 0;
            } else {
                r = topRadius;
            }
            for (int xx = x - r; xx <= x + r && free; xx++) {
                for (int zz = z - r; zz <= z + r && free; zz++) {
                    if (yy >= 0 && yy < Level::DEPTH) {
                        int tt = level->getTile(xx, yy, zz);
                        if (tt != 0 && tt != Tile::leaves->id) free = false;
                    } else {
                        free = false;
                    }
                }
            }
        }

        if (!free) return false;

        // must stand on ground
        int belowTile = level->getTile(x, y - 1, z);
        if ((belowTile != ((Tile*)Tile::grass)->id && belowTile != Tile::dirt->id) || y >= Level::DEPTH - treeHeight - 1) return false;

        placeBlock(level, x, y - 1, z, Tile::dirt->id);

        // place leaf top
        int currentRadius = 0;
        for (int yy = y + treeHeight; yy >= y + trunkHeight; yy--) {

            for (int xx = x - currentRadius; xx <= x + currentRadius; xx++) {
                int xo = xx - (x);
                for (int zz = z - currentRadius; zz <= z + currentRadius; zz++) {
                    int zo = zz - (z);
                    if (std::abs(xo) == currentRadius && std::abs(zo) == currentRadius && currentRadius > 0) continue;
                    if (!Tile::solid[level->getTile(xx, yy, zz)]) placeBlock(level, xx, yy, zz, Tile::leaves->id, LeafTile::EVERGREEN_LEAF);
                }
            }

            if (currentRadius >= 1 && yy == (y + trunkHeight + 1)) {
                currentRadius -= 1;
            } else if (currentRadius < topRadius) {
                currentRadius += 1;
            }
        }
        for (int hh = 0; hh < treeHeight - 1; hh++) {
            int t = level->getTile(x, y + hh, z);
            if (t == 0 || t == Tile::leaves->id) placeBlock(level, x, y + hh, z, Tile::treeTrunk->id, TreeTile::DARK_TRUNK);
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__PineFeature_H__*/
