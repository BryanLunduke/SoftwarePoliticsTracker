#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ReedsFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ReedsFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../material/Material.h"
#include "../../../../util/Random.h"

class ReedsFeature: public Feature
{
public:
	bool place(Level* level, Random* random, int x, int y, int z) {
        for (int i = 0; i < 20; i++) {
            int x2 = x + random->nextInt(4) - random->nextInt(4);
            int y2 = y;
            int z2 = z + random->nextInt(4) - random->nextInt(4);
            if (level->isEmptyTile(x2, y2, z2)) {
                if (level->getMaterial(x2-1, y2-1, z2) == Material::water || 
                    level->getMaterial(x2+1, y2-1, z2) == Material::water || 
                    level->getMaterial(x2, y2-1, z2-1) == Material::water || 
                    level->getMaterial(x2, y2-1, z2+1) == Material::water) {

                    int h = 2 + random->nextInt(random->nextInt(3) + 1);
                    for (int yy = 0; yy < h; yy++) {
                        if (Tile::reeds->canSurvive(level, x2, y2 + yy, z2)) {
                            level->setTileNoUpdate(x2, y2 + yy, z2, Tile::reeds->id);
                        }
                    }
                }
            }
        }

        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ReedsFeature_H__*/
