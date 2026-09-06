#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__OreFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__OreFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../material/Material.h"
#include "../../../../util/Mth.h"
#include "../../../../util/Random.h"

class OreFeature: public Feature {
    int tile;
    int count;

public:
	OreFeature(int tile, int count) {
        this->tile = tile;
        this->count = count;
    }

    bool place(Level* level, Random* random, int x, int y, int z) {

        float dir = random->nextFloat() * Mth::PI;

        float x0 = x + 8 + Mth::sin(dir) * count / 8;
        float x1 = x + 8 - Mth::sin(dir) * count / 8;
        float z0 = z + 8 + Mth::cos(dir) * count / 8;
        float z1 = z + 8 - Mth::cos(dir) * count / 8;

        float y0 = (float)(y + random->nextInt(3) + 2);
        float y1 = (float)(y + random->nextInt(3) + 2);


        for (int D = 0; D <= count; D++) {
			float d = (float) D;
            float xx = x0 + (x1 - x0) * d / count;
            float yy = y0 + (y1 - y0) * d / count;
            float zz = z0 + (z1 - z0) * d / count;

            float ss = random->nextFloat() * count / 16;
            float r = (Mth::sin(d * Mth::PI / count) + 1) * ss + 1;
            float hr = (Mth::sin(d * Mth::PI / count) + 1) * ss + 1;

            int xt0 = (int) (xx - r / 2);
            int yt0 = (int) (yy - hr / 2);
            int zt0 = (int) (zz - r / 2);

            int xt1 = (int) (xx + r / 2);
            int yt1 = (int) (yy + hr / 2);
            int zt1 = (int) (zz + r / 2);

            for (int x2 = xt0; x2 <= xt1; x2++) {
                float xd = ((x2 + 0.5f) - xx) / (r / 2);
                if (xd * xd < 1) {
                    for (int y2 = yt0; y2 <= yt1; y2++) {
                        float yd = ((y2 + 0.5f) - yy) / (hr / 2);
                        if (xd * xd + yd * yd < 1) {
                            for (int z2 = zt0; z2 <= zt1; z2++) {
                                float zd = ((z2 + 0.5f) - zz) / (r / 2);
                                if (xd * xd + yd * yd + zd * zd < 1) {
                                    if (level->getTile(x2, y2, z2) == Tile::rock->id) level->setTileNoUpdate(x2, y2, z2, tile);
                                }
                            }
                        }
                    }
                }
            }
        }


        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__OreFeature_H__*/
