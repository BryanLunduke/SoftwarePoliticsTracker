#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__CactusFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__CactusFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "../../../../util/Mth.h"

#include "../../Level.h"
#include "../../tile/CactusTile.h"
/* import net.minecraft.world.level.tile.* */

class CactusFeature: public Feature
{
public:
    bool place(Level* level, Random* random, int x, int y, int z) {
        for (int i = 0; i < 10; i++) {
            int x2 = x + random->nextInt(8) - random->nextInt(8);
            int y2 = y + random->nextInt(4) - random->nextInt(4);
            int z2 = z + random->nextInt(8) - random->nextInt(8);
            if (level->isEmptyTile(x2, y2, z2)) {
                int h = 1 + random->nextInt(random->nextInt(3) + 1);
                for (int yy = 0; yy < h; yy++) {
                    if (Tile::cactus->canSurvive(level, x2, y2+yy, z2)) {
                        //LOGI("Creating cactus part at  %d, %d, %d\n", x, y, z);
                        level->setTileNoUpdate(x2, y2+yy, z2, Tile::cactus->id);
                    }
                }
            }
        }

        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__CactusFeature_H__*/
