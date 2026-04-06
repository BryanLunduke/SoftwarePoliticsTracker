#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__FlowerFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__FlowerFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "../../../../util/Random.h"
#include "../../Level.h"

#include "../../tile/Bush.h"
#include "Feature.h"

class FlowerFeature: public Feature {
public:
	int tile;

    FlowerFeature(int tile) {
        this->tile = tile;
    }

    bool place(Level* level, Random* random, int x, int y, int z) {
        for (int i = 0; i < 64; i++) {
            int x2 = x + random->nextInt(8) - random->nextInt(8);
            int y2 = y + random->nextInt(4) - random->nextInt(4);
            int z2 = z + random->nextInt(8) - random->nextInt(8);
            if (level->isEmptyTile(x2, y2, z2)) {
                if (((Bush*) Tile::tiles[tile])->canSurvive(level, x2, y2, z2)) {
                    level->setTileNoUpdate(x2, y2, z2, tile);
                }
            }
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__FlowerFeature_H__*/
