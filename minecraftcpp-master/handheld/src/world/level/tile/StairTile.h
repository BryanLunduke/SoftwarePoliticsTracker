#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__StairTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__StairTile_H__

//package net.minecraft.world.level.tile;

#include <vector>

#include "Tile.h"
#include "../../../util/Mth.h"
#include "../../../util/Random.h"
#include "../../entity/Mob.h"

class StairTile: public Tile
{
    typedef Tile super;
    Tile* base;

	bool isClipping;
	int clipStep;
	int baseData;

public:

	static const int UPSIDEDOWN_BIT = 4;

	// the direction is the way going up (for normal non-upsidedown stairs)
	static const int DIR_EAST = 0;
	static const int DIR_WEST = 1;
	static const int DIR_SOUTH = 2;
	static const int DIR_NORTH = 3;

	static const int DEAD_SPACES[8][2];

    StairTile(int id, Tile* base)
    :   super(id, base->tex, base->material),
        base(base),
		isClipping(false),
		clipStep(0),
		baseData(0) // when needed in the future, set this to necessary texture data
    {
        setDestroyTime(base->destroySpeed);
        setExplodeable(base->explosionResistance / 3);
        setSoundType(*base->soundType);
		setLightBlock(255);
    }

	HitResult clip(Level* level, int xt, int yt, int zt, const Vec3& a, const Vec3& b);

    void updateShape(LevelSource* level, int x, int y, int z) {
		if (isClipping) {
			setShape(0.5f * (clipStep % 2), 0.5f * (clipStep / 2 % 2), 0.5f * (clipStep / 4 % 2), 0.5f + 0.5f * (clipStep % 2), 0.5f + 0.5f * (clipStep / 2 % 2), 0.5f + 0.5f * (clipStep / 4 % 2));
		} else {
			setShape(0, 0, 0, 1, 1, 1);
		}
    }

	void setBaseShape(LevelSource* level, int x, int y, int z) {
		int data = level->getData(x, y, z);

		if ((data & UPSIDEDOWN_BIT) != 0) {
			setShape(0, .5f, 0, 1, 1, 1);
		} else {
			setShape(0, 0, 0, 1, .5f, 1);
		}
	}

	static bool isStairs(int id) {
		return id > 0 && Tile::tiles[id]->getRenderShape() == Tile::SHAPE_STAIRS;
	}

	bool isLockAttached(LevelSource* level, int x, int y, int z, int data) {
		int lockTile = level->getTile(x, y, z);
		if (isStairs(lockTile) && level->getData(x, y, z) == data) {
			return true;
		}

		return false;
	}

	bool setStepShape(LevelSource* level, int x, int y, int z);
	bool setInnerPieceShape(LevelSource* level, int x, int y, int z);

//    AABB* getAABB(Level* level, int x, int y, int z) {
//        return super::getAABB(level, x, y, z);
////        return AABB.newTemp(x, y, z, x + 1, y + 1, z + 1);
//    }

    bool isSolidRender() {
        return false;
    }

    bool isCubeShaped() {
        return false;
    }

    int getRenderShape() {
        return Tile::SHAPE_STAIRS;
    }

    //bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
    //    return super::shouldRenderFace(level, x, y, z, face);
    //}

	void addAABBs(Level* level, int x, int y, int z, const AABB* box, std::vector<AABB>& boxes) {
		setBaseShape(level, x, y, z);
		super::addAABBs(level, x, y, z, box, boxes);

		bool checkInnerPiece = setStepShape(level, x, y, z);
		super::addAABBs(level, x, y, z, box, boxes);

		if (checkInnerPiece) {
			if (setInnerPieceShape(level, x, y, z)) {
				super::addAABBs(level, x, y, z, box, boxes);
			}
		}
        setShape(0, 0, 0, 1, 1, 1);
    }

    /*
     * void neighborChanged(Level level, int x, int y, int z, int type) {
     * if (level.isOnline) return; if (level.getMaterial(x, y + 1, z).isSolid())
     * { level.setTile(x, y, z, base.id); } else { checkStairs(level, x, y, z);
     * checkStairs(level, x + 1, y - 1, z); checkStairs(level, x - 1, y - 1, z);
     * checkStairs(level, x, y - 1, z - 1); checkStairs(level, x, y - 1, z + 1);
     * checkStairs(level, x + 1, y + 1, z); checkStairs(level, x - 1, y + 1, z);
     * checkStairs(level, x, y + 1, z - 1); checkStairs(level, x, y + 1, z + 1);
     * } base.neighborChanged(level, x, y, z, type); }
     */

    /** DELEGATES: **/

    void addLights(Level* level, int x, int y, int z) {
        base->addLights(level, x, y, z);
    }

    void animateTick(Level* level, int x, int y, int z, Random* random) {
        base->animateTick(level, x, y, z, random);
    }

    void attack(Level* level, int x, int y, int z, Player* player) {
        base->attack(level, x, y, z, player);
    }

    void destroy(Level* level, int x, int y, int z, int data) {
        base->destroy(level, x, y, z, data);
    }

    float getBrightness(LevelSource* level, int x, int y, int z) {
        return base->getBrightness(level, x, y, z);
    }

    float getExplosionResistance(Entity* source) {
        return base->getExplosionResistance(source);
    }

    int getRenderLayer() {
        return base->getRenderLayer();
    }

    int getResourceCount(Random* random) {
        return base->getResourceCount(random);
    }

    int getTexture(int face, int data) {
        return base->getTexture(face, baseData);
    }

    int getTexture(int face) {
        return base->getTexture(face, baseData);
    }

    int getTexture(LevelSource* level, int x, int y, int z, int face) {
        return base->getTexture(face, baseData);
    }

    int getTickDelay() {
        return base->getTickDelay();
    }

    AABB getTileAABB(Level* level, int x, int y, int z) {
        return base->getTileAABB(level, x, y, z);
    }

    void handleEntityInside(Level* level, int x, int y, int z, Entity* e, Vec3& current) {
        base->handleEntityInside(level, x, y, z, e, current);
    }

    bool mayPick() {
        return base->mayPick();
    }

    bool mayPick(int data, bool liquid) {
        return base->mayPick(data, liquid);
    }

    bool mayPlace(Level* level, int x, int y, int z, unsigned char face) {
        return base->mayPlace(level, x, y, z);
    }

    void onPlace(Level* level, int x, int y, int z) {
        neighborChanged(level, x, y, z, 0);
        base->onPlace(level, x, y, z);
    }

    void onRemove(Level* level, int x, int y, int z) {
        base->onRemove(level, x, y, z);
    }

    void prepareRender(Level* level, int x, int y, int z) {
        base->prepareRender(level, x, y, z);
    }

    void stepOn(Level* level, int x, int y, int z, Entity* entity) {
        base->stepOn(level, x, y, z, entity);
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        base->tick(level, x, y, z, random);
    }

    bool use(Level* level, int x, int y, int z, Player* player) {
        return base->use(level, x, y, z, player);
    }

    void wasExploded(Level* level, int x, int y, int z) {
        base->wasExploded(level, x, y, z);
    }

    void setPlacedBy(Level* level, int x, int y, int z, Mob* by) {
        int dir = (Mth::floor(by->yRot * 4 / (360) + 0.5f)) & 3;
		int usd = level->getData(x, y, z) & UPSIDEDOWN_BIT;

        if (dir == 0) level->setData(x, y, z, 2 | usd);
        if (dir == 1) level->setData(x, y, z, 1 | usd);
        if (dir == 2) level->setData(x, y, z, 3 | usd);
        if (dir == 3) level->setData(x, y, z, 0 | usd);
    }

	int getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue);

};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__StairTile_H__*/
