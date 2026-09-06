#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__FarmTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__FarmTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class FarmTile: public Tile {
public:
	FarmTile(int id)
	:	Tile(id, Material::dirt)
	{
        tex = 7 + 5 * 16;
        setTicking(true);
        setShape(0, 0, 0, 1, 15 / 16.0f, 1);
        setLightBlock(255);
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
		tmpBB.x0 = (float)x;
		tmpBB.y0 = (float)y;
		tmpBB.z0 = (float)z;
		tmpBB.x1 = (float)(x + 1);
		tmpBB.y1 = (float)(y + 1);
		tmpBB.z1 = (float)(z + 1);
		return &tmpBB;
    }

    bool isSolidRender() {
        return false;
    }

    bool isCubeShaped() {
        return false;
    }

    int getTexture(int face, int data) {
        if (face == 1 && data > 0) return tex - 1;
        if (face == 1) return tex;
        return 2;
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
		if (isNearWater(level, x, y, z)) {
			level->setData(x, y, z, 7);
		} else {
			int moisture = level->getData(x, y, z);
			if (moisture > 0) {
				level->setData(x, y, z, moisture - 1);
			} else {
				if (!isUnderCrops(level, x, y, z)) {
					level->setTile(x, y, z, Tile::dirt->id);
				}
			}
		}
    }

	void fallOn(Level* level, int x, int y, int z, Entity* entity, float fallDistance) {
		if (!level->isClientSide && level->random.nextFloat() < (fallDistance - .5f)) {
			level->setTile(x, y, z, Tile::dirt->id);
		}
	}

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        Tile::neighborChanged(level, x, y, z, type);
        const Material* above = level->getMaterial(x, y + 1, z);
        if (above->isSolid()) {
            level->setTile(x, y, z, Tile::dirt->id);
        }
    }

    bool blocksLight() {
        return true;
    }
    
    int getResource(int data, Random* random) {
        return Tile::dirt->getResource(0, random);
    }

private:
    bool isUnderCrops(Level* level, int x, int y, int z) {
        int r = 0;
        for (int xx = x - r; xx <= x + r; xx++)
            for (int zz = z - r; zz <= z + r; zz++) {
                if (level->getTile(xx, y + 1, zz) == Tile::crops->id) {
                    return true;
                }
            }
        return false;
    }

    bool isNearWater(Level* level, int x, int y, int z) {
        for (int xx = x - 4; xx <= x + 4; xx++) {
            for (int yy = y; yy <= y + 1; yy++) {
                for (int zz = z - 4; zz <= z + 4; zz++) {
                    if (level->getMaterial(xx, yy, zz) == Material::water) {
                        return true;
                    }
                }
			}
		}
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__FarmTile_H__*/
