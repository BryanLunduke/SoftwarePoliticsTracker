#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LadderTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LadderTile_H__

//package net.minecraft.world.level->tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class LadderTile: public Tile
{
	typedef Tile super;

public:
    LadderTile(int id, int tex)
    :	super(id, tex, Material::decoration)
	{
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        int dir = level->getData(x, y, z);
        float r = 2 / 16.0f;

        if (dir == 2) this->setShape(0, 0, 1 - r, 1, 1, 1);
        if (dir == 3) this->setShape(0, 0, 0, 1, 1, r);
        if (dir == 4) this->setShape(1 - r, 0, 0, 1, 1, 1);
        if (dir == 5) this->setShape(0, 0, 0, r, 1, 1);

        return super::getAABB(level, x, y, z);
    }

    AABB getTileAABB(Level* level, int x, int y, int z) {
        int dir = level->getData(x, y, z);
        float r = 2 / 16.0f;

        if (dir == 2) this->setShape(0, 0, 1 - r, 1, 1, 1);
        if (dir == 3) this->setShape(0, 0, 0, 1, 1, r);
        if (dir == 4) this->setShape(1 - r, 0, 0, 1, 1, 1);
        if (dir == 5) this->setShape(0, 0, 0, r, 1, 1);

        return super::getTileAABB(level, x, y, z);
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
        return Tile::SHAPE_LADDER;
    }
	int getRenderLayer() {
        return Tile::RENDERLAYER_ALPHATEST;
    }

    bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
        if (level->isSolidBlockingTile(x - 1, y, z)) {
            return true;
        } else if (level->isSolidBlockingTile(x + 1, y, z)) {
            return true;
        } else if (level->isSolidBlockingTile(x, y, z - 1)) {
            return true;
        } else if (level->isSolidBlockingTile(x, y, z + 1)) {
            return true;
        }
        return false;
    }

	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue)
	{
		int dir = itemValue;

		if ((dir == 0 || face == 2) && level->isSolidBlockingTile(x, y, z + 1)) dir = 2;
		if ((dir == 0 || face == 3) && level->isSolidBlockingTile(x, y, z - 1)) dir = 3;
		if ((dir == 0 || face == 4) && level->isSolidBlockingTile(x + 1, y, z)) dir = 4;
		if ((dir == 0 || face == 5) && level->isSolidBlockingTile(x - 1, y, z)) dir = 5;

		return dir;
	}

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        int face = level->getData(x, y, z);
        bool ok = false;

        if (face == 2 && level->isSolidBlockingTile(x, y, z + 1)) ok = true;
        if (face == 3 && level->isSolidBlockingTile(x, y, z - 1)) ok = true;
        if (face == 4 && level->isSolidBlockingTile(x + 1, y, z)) ok = true;
        if (face == 5 && level->isSolidBlockingTile(x - 1, y, z)) ok = true;
        if (!ok) {
            //spawnResources(level, x, y, z, face); //@crafting
			popResource(level, x, y, z, ItemInstance(Tile::ladder));
            level->setTile(x, y, z, 0);
        }

        super::neighborChanged(level, x, y, z, type);
    }

    int getResourceCount(Random* random) {
        return 1;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LadderTile_H__*/
