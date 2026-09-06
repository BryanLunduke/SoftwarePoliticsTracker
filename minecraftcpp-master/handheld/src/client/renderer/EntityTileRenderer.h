#ifndef NET_MINECRAFT_CLIENT_RENDERER__EntityTileRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER__EntityTileRenderer_H__

//package net.minecraft.client.renderer;

#include "../../world/level/tile/entity/ChestTileEntity.h"
#include "tileentity/TileEntityRenderDispatcher.h"

class Tile;

class EntityTileRenderer
{
public:
    static EntityTileRenderer* instance;

    void render(Tile* tile, int data, float brightness);

    ChestTileEntity chest;
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__EntityTileRenderer_H__*/
