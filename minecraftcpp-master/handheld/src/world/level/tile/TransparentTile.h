#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TransparentTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TransparentTile_H__

//package net.minecraft.world.level.tile;

#include "Tile.h"
#include "../LevelSource.h"
class Material;

class TransparentTile: public Tile
{
public:
    /*protected*/
	bool allowSame;
    
    /*protected*/
	TransparentTile(int id, int tex, const Material* material, bool allowSame)
	:	Tile(id, tex, material)
    {
        this->allowSame = allowSame;
    }
    
    bool isSolidRender()
    {
        return false;
    }
    
    bool shouldRenderFace(LevelSource* level, int x, int y, int z, int face)
    {
        int id = level->getTile(x, y, z);
        if (!allowSame && id == this->id) return false;
        return Tile::shouldRenderFace(level, x, y, z, face);
    }     

    bool blocksLight() {
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TransparentTile_H__*/
