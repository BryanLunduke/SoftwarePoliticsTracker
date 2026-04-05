#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TorchTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TorchTile_H__

//package net.minecraft.world.level->tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class TorchTile: public Tile
{
	typedef Tile super;

public:
    TorchTile(int id, int tex)
	:	super(id, tex, Material::decoration)
	{
        this->setTicking(true);
    }

    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    bool isSolidRender() {
        return false;
    }

    bool isCubeShaped() {
        return false;
    }

    int getRenderShape() {
        return Tile::SHAPE_TORCH;
    }

    int getRenderLayer() {
        return Tile::RENDERLAYER_ALPHATEST;
    }
    
    bool mayPlace(Level* level, int x, int y, int z) {
        if (level->isSolidBlockingTile(x - 1, y, z)) {
            return true;
        } else if (level->isSolidBlockingTile(x + 1, y, z)) {
            return true;
        } else if (level->isSolidBlockingTile(x, y, z - 1)) {
            return true;
        } else if (level->isSolidBlockingTile(x, y, z + 1)) {
            return true;
        } else if (isConnection(level, x, y - 1, z)) {
            return true;
        }
        return false;
    }

	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue)
	{
		int dir = itemValue;

		if (face == 1 && isConnection(level, x, y - 1, z)) dir = 5;
		if (face == 2 && level->isSolidBlockingTile(x, y, z + 1)) dir = 4;
		if (face == 3 && level->isSolidBlockingTile(x, y, z - 1)) dir = 3;
		if (face == 4 && level->isSolidBlockingTile(x + 1, y, z)) dir = 2;
		if (face == 5 && level->isSolidBlockingTile(x - 1, y, z)) dir = 1;

		return dir;
	}

    void tick(Level* level, int x, int y, int z, Random* random) {
        super::tick(level, x, y, z, random);
        if (level->getData(x, y, z) == 0) onPlace(level, x, y, z);
    }

    void onPlace(Level* level, int x, int y, int z) {
        if (level->isSolidBlockingTile(x - 1, y, z)) {
            level->setData(x, y, z, 1);
        } else if (level->isSolidBlockingTile(x + 1, y, z)) {
            level->setData(x, y, z, 2);
        } else if (level->isSolidBlockingTile(x, y, z - 1)) {
            level->setData(x, y, z, 3);
        } else if (level->isSolidBlockingTile(x, y, z + 1)) {
            level->setData(x, y, z, 4);
        } else if (isConnection(level, x, y - 1, z)) {
            level->setData(x, y, z, 5);
        }
        checkCanSurvive(level, x, y, z);
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        if (checkCanSurvive(level, x, y, z)) {
            int dir = level->getData(x, y, z);
            bool replace = false;

            if (!level->isSolidBlockingTile(x - 1, y, z) && dir == 1) replace = true;
            if (!level->isSolidBlockingTile(x + 1, y, z) && dir == 2) replace = true;
            if (!level->isSolidBlockingTile(x, y, z - 1) && dir == 3) replace = true;
            if (!level->isSolidBlockingTile(x, y, z + 1) && dir == 4) replace = true;
            if (!isConnection(level, x, y - 1, z) && dir == 5) replace = true;

            if (replace) {
                this->spawnResources(level, x, y, z, level->getData(x, y, z));
                level->setTile(x, y, z, 0);
            }
        }
    }

    HitResult clip(Level* level, int x, int y, int z, const Vec3& a, const Vec3& b) {
        int dir = level->getData(x, y, z) & 7;

        float r = 0.15f;
        if (dir == 1) {
            setShape(0, 0.2f, 0.5f - r, r * 2, 0.8f, 0.5f + r);
        } else if (dir == 2) {
            setShape(1 - r * 2, 0.2f, 0.5f - r, 1, 0.8f, 0.5f + r);
        } else if (dir == 3) {
            setShape(0.5f - r, 0.2f, 0, 0.5f + r, 0.8f, r * 2);
        } else if (dir == 4) {
            setShape(0.5f - r, 0.2f, 1 - r * 2, 0.5f + r, 0.8f, 1);
        } else {
            r = 0.1f;
            setShape(0.5f - r, 0.0f, 0.5f - r, 0.5f + r, 0.6f, 0.5f + r);
        }

        return super::clip(level, x, y, z, a, b);
    }

    void animateTick(Level* level, int xt, int yt, int zt, Random* random) {
        int dir = level->getData(xt, yt, zt);
        float x = xt + 0.5f;
        float y = yt + 0.7f;
        float z = zt + 0.5f;
        float h = 0.22f;
        float r = 0.27f;
        if (dir == 1) {
            level->addParticle(PARTICLETYPE(smoke), x - r, y + h, z, 0, 0, 0);
            level->addParticle(PARTICLETYPE(flame), x - r, y + h, z, 0, 0, 0);
        } else if (dir == 2) {
            level->addParticle(PARTICLETYPE(smoke), x + r, y + h, z, 0, 0, 0);
            level->addParticle(PARTICLETYPE(flame), x + r, y + h, z, 0, 0, 0);
        } else if (dir == 3) {
            level->addParticle(PARTICLETYPE(smoke), x, y + h, z - r, 0, 0, 0);
            level->addParticle(PARTICLETYPE(flame), x, y + h, z - r, 0, 0, 0);
        } else if (dir == 4) {
            level->addParticle(PARTICLETYPE(smoke), x, y + h, z + r, 0, 0, 0);
            level->addParticle(PARTICLETYPE(flame), x, y + h, z + r, 0, 0, 0);
        } else {
            level->addParticle(PARTICLETYPE(smoke), x, y, z, 0, 0, 0);
            level->addParticle(PARTICLETYPE(flame), x, y, z, 0, 0, 0);
        }
    }
private:
    bool isConnection(Level* level, int x, int y, int z) {
        if (level->isSolidBlockingTile(x, y, z)) {
            return true;
        }
        int tile = level->getTile(x, y, z);
        if (tile == Tile::fence->id || tile == Tile::glass->id /*|| tile == Tile::cobbleWall->id*/) {
            return true;
        }
        return false;
    }

	bool checkCanSurvive(Level* level, int x, int y, int z) {
		if (!mayPlace(level, x, y, z)) {
			if (level->getTile(x, y, z) == id) {
				spawnResources(level, x, y, z, level->getData(x, y, z));
				level->setTile(x, y, z, 0);
			}
			return false;
		}
		return true;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TorchTile_H__*/
