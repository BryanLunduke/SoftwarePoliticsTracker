#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__SignTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__SignTile_H__

//package net.minecraft.world.level.tile;

#include "EntityTile.h"
#include "../../item/Item.h"
#include "../material/Material.h"
#include "entity/TileEntity.h"
#include "../../phys/AABB.h"

class SignTile: public EntityTile
{
	typedef EntityTile super;
public:
    SignTile(int id, int tileEntityClassId, bool onGround)
	:	super(id, Material::wood),
		tileEntityClassId(tileEntityClassId),
        onGround(onGround)
	{
        tex = 4;
        float r = 4 / 16.0f;
        float h = 16 / 16.0f;
        this->setShape(0.5f - r, 0, 0.5f - r, 0.5f + r, h, 0.5f + r);
    }

    /*@Override*/
    AABB* getAABB(Level* level, int x, int y, int z) {
        return NULL;
    }

    /*@Override*/
    AABB getTileAABB(Level* level, int x, int y, int z) {
        updateShape(level, x, y, z);
        return super::getTileAABB(level, x, y, z);
    }

    /*@Override*/
    void updateShape(LevelSource* level, int x, int y, int z) {
        if (onGround) return;

        int face = level->getData(x, y, z);

        float h0 = (4 + 0.5f) / 16.0f;
        float h1 = (12 + 0.5f) / 16.0f;
        float w0 = 0 / 16.0f;
        float w1 = 16 / 16.0f;

        float d0 = 2 / 16.0f;

        setShape(0, 0, 0, 1, 1, 1);
        if (face == 2) setShape(w0, h0, 1 - d0, w1, h1, 1);
        if (face == 3) setShape(w0, h0, 0, w1, h1, d0);
        if (face == 4) setShape(1 - d0, h0, w0, 1, h1, w1);
        if (face == 5) setShape(0, h0, w0, d0, h1, w1);
    }

    /*@Override*/
	int getRenderShape() {
        return Tile::SHAPE_INVISIBLE;
    }

    /*@Override*/
    bool isCubeShaped() {
        return false;
    }

    /*@Override*/
    bool isPathfindable(LevelSource* level, int x, int y, int z) {
        return true;
    }

    /*@Override*/
    bool isSolidRender() {
        return false;
    }

    /*@Override*/
    TileEntity* newTileEntity() {
        return TileEntityFactory::createTileEntity(tileEntityClassId);
    }

    /*@Override*/
    int getResource(int data, Random* random/*, int playerBonusLevel*/) {
        return Item::sign->id;
    }

    /*@Override*/
    void neighborChanged(Level* level, int x, int y, int z, int type) {
        bool remove = false;

        if (onGround) {
            if (!level->getMaterial(x, y - 1, z)->isSolid()) remove = true;
        } else {
            int face = level->getData(x, y, z);
            remove = true;
            if (face == 2 && level->getMaterial(x, y, z + 1)->isSolid()) remove = false;
            if (face == 3 && level->getMaterial(x, y, z - 1)->isSolid()) remove = false;
            if (face == 4 && level->getMaterial(x + 1, y, z)->isSolid()) remove = false;
            if (face == 5 && level->getMaterial(x - 1, y, z)->isSolid()) remove = false;
        }
        if (remove) {
            spawnResources(level, x, y, z, level->getData(x, y, z), 0);
            level->setTile(x, y, z, 0);
        }
        super::neighborChanged(level, x, y, z, type);
    }
private:
    bool onGround;
    int tileEntityClassId;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__SignTile_H__*/
