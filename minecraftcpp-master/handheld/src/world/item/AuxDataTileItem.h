#ifndef NET_MINECRAFT_WORLD_ITEM__AuxDataTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__AuxDataTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "../level/tile/Tile.h"

class AuxDataTileItem: public TileItem
{
    typedef TileItem super;
public:
    AuxDataTileItem(int id, Tile* parentTile)
    :   super(id),
        parentTile(parentTile)
    {
        setMaxDamage(0);
        setStackedByData(true);
    }

    /*@Override*/
    int getIcon(int itemAuxValue) {
        return parentTile->getTexture(2, itemAuxValue);
    }

    /*@Override*/
    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }
private:
    Tile* parentTile;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__AuxDataTileItem_H__*/
