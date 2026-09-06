#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__ReedTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__ReedTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class ReedTile: public Tile
{
public:
	ReedTile(int id, int tex)
	: Tile(id, Material::plant)
	{
        this->tex = tex;
        float ss = 6 / 16.0f;
        this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, 1, 0.5f + ss);
        this->setTicking(true);
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->isEmptyTile(x, y + 1, z)) {
            int height = 1;
            while (level->getTile(x, y - height, z) == id) {
                height++;
            }
            if (height < 3) {
                int age = level->getData(x, y, z);
                if (age == 15) {
                    level->setTile(x, y + 1, z, id);
                    level->setData(x, y, z, 0);
                } else {
                    level->setData(x, y, z, age + 1);
                }
            }
        }
    }

    bool mayPlace(Level* level, int x, int y, int z) {
        int below = level->getTile(x, y - 1, z);
        if (below == id) return true;
        if (below != ((Tile*)Tile::grass)->id && below != Tile::dirt->id && below != Tile::sand->id) return false;
        if (level->getMaterial(x - 1, y - 1, z) == Material::water) return true;
        if (level->getMaterial(x + 1, y - 1, z) == Material::water) return true;
        if (level->getMaterial(x, y - 1, z - 1) == Material::water) return true;
        if (level->getMaterial(x, y - 1, z + 1) == Material::water) return true;
        return false;
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        checkAlive(level, x, y, z);
    }

    bool canSurvive(Level* level, int x, int y, int z) {
        return mayPlace(level, x, y, z);
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    int getResource(int data, Random* random) {
        return Item::reeds->id;
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
        return Tile::SHAPE_CROSS_TEXTURE;
    }

	int getRenderLayer() {
        return Tile::RENDERLAYER_ALPHATEST;
    }

protected:
    const void checkAlive(Level* level, int x, int y, int z) {
        if (!canSurvive(level, x, y, z)) {
			popResource(level, x, y, z, ItemInstance(Item::reeds));
            level->setTile(x, y, z, 0);
        }
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__ReedTile_H__*/
