#ifndef NET_MINECRAFT_WORLD_ITEM__SaplingTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SaplingTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "../level/tile/Tile.h"

class SaplingTileItem: public TileItem
{
    typedef TileItem super;
public:
    SaplingTileItem(int id)
    :   super(id)
    {
        setMaxDamage(0);
        setStackedByData(true);
    }

    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }

    int getIcon(int itemAuxValue) {
        return Tile::sapling->getTexture(0, itemAuxValue);
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SaplingTileItem_H__*/
