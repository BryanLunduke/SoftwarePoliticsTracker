#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__HalfTransparentTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__HalfTransparentTile_H__

//package net.minecraft.world.level.tile;

#include "../LevelSource.h"
#include "../material/Material.h"

class HalfTransparentTile: public Tile {
    bool allowSame;

public:
    /*protected*/
	HalfTransparentTile(int id, int tex, const Material* material, bool allowSame)
	:	Tile(id, tex, material)
	{
        this->allowSame = allowSame;
    }

	int getRenderLayer() {
        return Tile::RENDERLAYER_BLEND;
    }

    bool isSolidRender() {
        return false;
    }

    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face) {
        int id = level->getTile(x, y, z);
        if (!allowSame && id == this->id) return false;
        return Tile::shouldRenderFace(level, x, y, z, face);
    }

    bool blocksLight() {
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__HalfTransparentTile_H__*/
