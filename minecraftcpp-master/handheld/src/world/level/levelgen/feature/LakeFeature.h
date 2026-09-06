#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__LakeFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__LakeFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"
#include "../../Level.h"
#include "../../tile/Tile.h"
#include "../../material/Material.h"
#include "../../../../util/Mth.h"
#include "../../../../util/Random.h"

class LakeFeature: public Feature
{
    int tile;

public:
    LakeFeature(int tile_)
	:	tile(tile_)
	{
    }

    bool place(Level* level, Random* random, int x, int y, int z) {
        x -= 8;
        z -= 8;
        while (y > 0 && level->isEmptyTile(x, y, z))
            y--;

        y -= 4;

		const int size = 16 * 16 * 8;
        bool grid[size];
		for (int i = 0; i < size; ++i)
			grid[i] = false;

        int spots = random->nextInt(4) + 4;
        for (int i = 0; i < spots; i++) {
            float xr = random->nextFloat() * 6 + 3;
            float yr = random->nextFloat() * 4 + 2;
            float zr = random->nextFloat() * 6 + 3;

            float xp = random->nextFloat() * (16 - xr - 2) + 1 + xr / 2;
            float yp = random->nextFloat() * (8 - yr - 4) + 2 + yr / 2;
            float zp = random->nextFloat() * (16 - zr - 2) + 1 + zr / 2;

            for (int xx = 1; xx < 15; xx++) {
                for (int zz = 1; zz < 15; zz++) {
                    for (int yy = 1; yy < 7; yy++) {
                        float xd = ((xx - xp) / (xr / 2));
                        float yd = ((yy - yp) / (yr / 2));
                        float zd = ((zz - zp) / (zr / 2));
                        float d = xd * xd + yd * yd + zd * zd;
                        if (d < 1) grid[((xx) * 16 + (zz)) * 8 + (yy)] = true;
                    }
                }
            }
        }

        for (int xx = 0; xx < 16; xx++) {
            for (int zz = 0; zz < 16; zz++) {
                for (int yy = 0; yy < 8; yy++) {
                    bool check = !grid[((xx) * 16 + (zz)) * 8 + (yy)] && (false// 
                            || (xx < 15 && grid[((xx + 1) * 16 + (zz)) * 8 + (yy)])//
                            || (xx > 0 && grid[((xx - 1) * 16 + (zz)) * 8 + (yy)])//
                            || (zz < 15 && grid[((xx) * 16 + (zz + 1)) * 8 + (yy)])//
                            || (zz > 0 && grid[((xx) * 16 + (zz - 1)) * 8 + (yy)])//
                            || (yy < 7 && grid[((xx) * 16 + (zz)) * 8 + (yy + 1)])//
                    || (yy > 0 && grid[((xx) * 16 + (zz)) * 8 + (yy - 1)]));

                    if (check) {
                        const Material* m = level->getMaterial(x + xx, y + yy, z + zz);
                        if (yy >= 4 && m->isLiquid()) return false;
                        if (yy < 4 && (!m->isSolid() && level->getTile(x + xx, y + yy, z + zz) != tile)) return false;

                    }
                }
            }
        }

        for (int xx = 0; xx < 16; xx++) {
            for (int zz = 0; zz < 16; zz++) {
                for (int yy = 0; yy < 8; yy++) {
                    if (grid[((xx) * 16 + (zz)) * 8 + (yy)]) {
                        level->setTileNoUpdate(x + xx, y + yy, z + zz, yy >= 4 ? 0 : tile);
                    }
                }
            }
        }

        for (int xx = 0; xx < 16; xx++) {
            for (int zz = 0; zz < 16; zz++) {
                for (int yy = 4; yy < 8; yy++) {
                    if (grid[((xx) * 16 + (zz)) * 8 + (yy)]) {
                        if (level->getTile(x + xx, y + yy - 1, z + zz) == Tile::dirt->id && level->getBrightness(LightLayer::Sky, x + xx, y + yy, z + zz) > 0) {
                            level->setTileNoUpdate(x + xx, y + yy - 1, z + zz, Tile::grass->id);
                        }
                    }
                }
            }
        }

        if (Tile::tiles[tile]->material == Material::lava) {
            for (int xx = 0; xx < 16; xx++) {
                for (int zz = 0; zz < 16; zz++) {
                    for (int yy = 0; yy < 8; yy++) {
                        bool check = !grid[((xx) * 16 + (zz)) * 8 + (yy)] && (false// 
                                || (xx < 15 && grid[((xx + 1) * 16 + (zz)) * 8 + (yy)])//
                                || (xx > 0 && grid[((xx - 1) * 16 + (zz)) * 8 + (yy)])//
                                || (zz < 15 && grid[((xx) * 16 + (zz + 1)) * 8 + (yy)])//
                                || (zz > 0 && grid[((xx) * 16 + (zz - 1)) * 8 + (yy)])//
                                || (yy < 7 && grid[((xx) * 16 + (zz)) * 8 + (yy + 1)])//
                        || (yy > 0 && grid[((xx) * 16 + (zz)) * 8 + (yy - 1)]));

                        if (check) {
                            if ((yy<4 || random->nextInt(2)!=0) && level->getMaterial(x + xx, y + yy, z + zz)->isSolid()) {
                                level->setTileNoUpdate(x + xx, y + yy, z + zz, Tile::rock->id);
                            }
                        }
                    }
                }
            }
        }
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__LakeFeature_H__*/
