#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__CactusTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__CactusTile_H__

//package net.minecraft.world.level->tile;

//#include "world/damagesource/DamageSource.h"
#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/Entity.h"
#include "../../phys/AABB.h"

class Random;

class CactusTile: public Tile
{
    typedef Tile super;

public:
    CactusTile(int id, int tex)
    :   super(id, tex, Material::cactus)
    {
        setTicking(true);
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->isEmptyTile(x, y + 1, z)) {
            int height = 1;
            while (level->getTile(x, y - height, z) == id) {
                height++;
            }
            if (height < 3) {
                int age = level->getData(x, y, z);
				// It takes way to long on pocket edition becuase of fewer ticks
                if (age >= /*15*/10) {
                    level->setTile(x, y + 1, z, id);
                    level->setData(x, y, z, 0);
                } else {
                    level->setData(x, y, z, age + 1);
                }
            }
        }
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        float r = 1 / 16.0f;
        tmpBB.x0 = x + r;
        tmpBB.y0 = (float)y;
        tmpBB.z0 = z + r;
        tmpBB.x1 = x + 1 - r;
        tmpBB.y1 = y + 1 - r;
        tmpBB.z1 = z + 1 - r;
        return &tmpBB;
    }

    AABB getTileAABB(Level* level, int x, int y, int z) {
        float r = 1 / 16.0f;
        return AABB((float)x + r, (float)y, (float)z + r, (float)x + 1 - r, (float)y + 1, (float)z + 1 - r);
    }

    int getTexture(int face) {
        if (face == 1) return tex - 1;
        if (face == 0) return tex + 1;
        else return tex;
    }

    bool isCubeShaped() {
        return false;
    }

    bool isSolidRender() {
        return false;
    }

    int getRenderShape() {
        return Tile::SHAPE_CACTUS;
    }
    
    int getRenderLayer() {
        return Tile::RENDERLAYER_ALPHATEST;
    }

    bool mayPlace(Level* level, int x, int y, int z) {
        if (!super::mayPlace(level, x, y, z)) return false;

        return canSurvive(level, x, y, z);
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        if (!canSurvive(level, x, y, z)) {
			// Should always spawn a cactus when this happens
            this->spawnResources(level, x, y, z, level->getData(x, y, z), 1);
            level->setTile(x, y, z, 0);
        }
    }

    bool canSurvive(Level* level, int x, int y, int z) {
        if (level->getMaterial(x - 1, y, z)->isSolid()) return false;
        if (level->getMaterial(x + 1, y, z)->isSolid()) return false;
        if (level->getMaterial(x, y, z - 1)->isSolid()) return false;
        if (level->getMaterial(x, y, z + 1)->isSolid()) return false;
        int below = level->getTile(x, y - 1, z);
        return below == Tile::cactus->id || below == Tile::sand->id;
    }

    void entityInside(Level* level, int x, int y, int z, Entity* entity) {
        entity->hurt(NULL/*DamageSource.cactus*/, 1);
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__CactusTile_H__*/
