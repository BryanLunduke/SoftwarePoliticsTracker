#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__FenceTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__FenceTile_H__

//package net.minecraft.world.level->tile;

#include "Tile.h"
/* import net.minecraft.world.level->* */
#include "../material/Material.h"
#include "../../phys/AABB.h"

class FenceTile: public Tile
{
	typedef Tile super;
public:
    FenceTile(int id, int tex)
    :   super(id, tex, Material::wood)
    {
    }

    FenceTile(int id, int tex, const Material* material)
    :   super(id, tex, material)
    {
    }

    bool mayPlace(Level* level, int x, int y, int z) {
        return super::mayPlace(level, x, y, z);
    }

    /*@Override*/
    AABB* getAABB(Level* level, int x, int y, int z) {
        bool n = connectsTo(level, x, y, z - 1);
        bool s = connectsTo(level, x, y, z + 1);
        bool w = connectsTo(level, x - 1, y, z);
        bool e = connectsTo(level, x + 1, y, z);

        float west = 6.0f / 16.0f;
        float east = 10.0f / 16.0f;
        float north = 6.0f / 16.0f;
        float south = 10.0f / 16.0f;

        if (n) {
            north = 0;
        }
        if (s) {
            south = 1;
        }
        if (w) {
            west = 0;
        }
        if (e) {
            east = 1;
        }

		tmpBB.set((float)x + west, (float)y, (float)z + north, (float)x + east, (float)y + 1.5f, (float)z + south);
		return &tmpBB;
    }

    /*@Override*/
    void updateShape(LevelSource* level, int x, int y, int z) {
        bool n = connectsTo(level, x, y, z - 1);
        bool s = connectsTo(level, x, y, z + 1);
        bool w = connectsTo(level, x - 1, y, z);
        bool e = connectsTo(level, x + 1, y, z);

        float west = 6.0f / 16.0f;
        float east = 10.0f / 16.0f;
        float north = 6.0f / 16.0f;
        float south = 10.0f / 16.0f;

        if (n) {
            north = 0;
        }
        if (s) {
            south = 1;
        }
        if (w) {
            west = 0;
        }
        if (e) {
            east = 1;
        }

        setShape(west, 0, north, east, 1.0f, south);
    }

    bool blocksLight() {
        return false;
    }

    bool isSolidRender() {
        return false;
    }

    bool isCubeShaped() {
        return false;
    }

    int getRenderShape() {
        return Tile::SHAPE_FENCE;
    }

    bool connectsTo(LevelSource* level, int x, int y, int z) {
        int tile = level->getTile(x, y, z);
        if (tile == id || tile == Tile::fenceGate->id) {
            return true;
        }
        Tile* tileInstance = Tile::tiles[tile];
        if (tileInstance != NULL) {
            if (tileInstance->material->isSolidBlocking() && tileInstance->isCubeShaped()) {
                return tileInstance->material != Material::vegetable;
            }
        }
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL->TILE__FenceTile_H__*/
