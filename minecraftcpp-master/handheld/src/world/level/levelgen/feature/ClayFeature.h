#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ClayFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ClayFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../material/Material.h"
#include "../../../../util/Mth.h"
#include "../../../../util/Random.h"

class ClayFeature: public Feature
{
    int tile;
    int count;

public:
    ClayFeature(int count) {
        this->tile = Tile::clay->id;
        this->count = count;
    }

    bool place(Level* level, Random* random, int x, int y, int z) {
        if (level->getMaterial(x, y, z) != Material::water) return false;

        float dir = random->nextFloat() * Mth::PI;

        float x0 = x + 8 + sin(dir) * count / 8;
        float x1 = x + 8 - sin(dir) * count / 8;
        float z0 = z + 8 + cos(dir) * count / 8;
        float z1 = z + 8 - cos(dir) * count / 8;

        float y0 = (float)(y + random->nextInt(3) + 2);
        float y1 = (float)(y + random->nextInt(3) + 2);

        for (int d = 0; d <= count; d++) {
            float xx = x0 + (x1 - x0) * d / count;
            float yy = y0 + (y1 - y0) * d / count;
            float zz = z0 + (z1 - z0) * d / count;

            float ss = random->nextFloat() * (float)(count >> 4);
            float r = (sin(d * Mth::PI / count) + 1) * ss + 1;
            float hr = (sin(d * Mth::PI / count) + 1) * ss + 1;

            for (int x2 = (int) (xx - r / 2); x2 <= (int) (xx + r * 0.5f); x2++)
                for (int y2 = (int) (yy - hr / 2); y2 <= (int) (yy + hr * 0.5f); y2++)
                    for (int z2 = (int) (zz - r / 2); z2 <= (int) (zz + r * 0.5f); z2++) {
                        float xd = ((x2 + 0.5f) - xx) / (r * 0.5f);
                        float yd = ((y2 + 0.5f) - yy) / (hr * 0.5f);
                        float zd = ((z2 + 0.5f) - zz) / (r * 0.5f);
                        if (xd * xd + yd * yd + zd * zd < 1) {
                            int t = level->getTile(x2, y2, z2);
                            if (t == Tile::sand->id) level->setTileNoUpdate(x2, y2, z2, tile);
                        }
                    }
        }

        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__ClayFeature_H__*/
