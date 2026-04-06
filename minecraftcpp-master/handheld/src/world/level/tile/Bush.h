#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__Bush_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__Bush_H__

//package net.minecraft.world.level.tile;

#include "../Level.h"
#include "Tile.h"
#include "../material/Material.h"

class Bush: public Tile
{
public:
	Bush(int id, int tex)
	:	Tile(id, Material::plant)
	{
		this->tex = tex;
		setTicking(true);
		float ss = 0.2f;
		this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 3, 0.5f + ss);
	}
	Bush(int id, int tex, const Material* material) : Tile(id, material) {
		this->tex = tex;
		setTicking(true);
		float ss = 0.2f;
		this->setShape(0.5f - ss, 0, 0.5f - ss, 0.5f + ss, ss * 3, 0.5f + ss);
	}

	bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
        return mayPlaceOn(level->getTile(x, y - 1, z));
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        Tile::neighborChanged(level, x, y, z, type);
        checkAlive(level, x, y, z);
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        checkAlive(level, x, y, z);
    }

    bool canSurvive(Level* level, int x, int y, int z) {
        return (level->getRawBrightness(x, y, z)>=8 || level->canSeeSky(x, y, z)) && mayPlaceOn(level->getTile(x, y - 1, z));
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
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
    virtual bool mayPlaceOn(int tile) {
        return tile == ((Tile*)Tile::grass)->id || tile == Tile::dirt->id || tile == Tile::farmland->id;
    }

    const void checkAlive(Level* level, int x, int y, int z) {
        if (!canSurvive(level, x, y, z)) {
            this->spawnResources(level, x, y, z, level->getData(x, y, z));
            level->setTile(x, y, z, 0);
			//printf("died! @ %d,%d,%d\n", x, y, z);
        }
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__Bush_H__*/
