#ifndef NET_MINECRAFT_WORLD_ITEM__LeafTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__LeafTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "../level/tile/LeafTile.h"
#include "../level/FoliageColor.h"

class LeafTileItem: public TileItem
{
    typedef TileItem super;
public:
    LeafTileItem(int id)
    :   super(id)
    {
        setMaxDamage(0);
        setStackedByData(true);
    }

    int getLevelDataForAuxValue(int auxValue) {
        return auxValue | LeafTile::PERSISTENT_LEAF_BIT;
    }

    int getIcon(int itemAuxValue) {
        return Tile::leaves->getTexture(0, itemAuxValue);
    }

    int getColor(int data) {
        if ((data & LeafTile::EVERGREEN_LEAF) == LeafTile::EVERGREEN_LEAF) {
            return FoliageColor::getEvergreenColor();
        }
        if ((data & LeafTile::BIRCH_LEAF) == LeafTile::BIRCH_LEAF) {
            return FoliageColor::getBirchColor();
        }
        return FoliageColor::getDefaultColor();
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__LeafTileItem_H__*/
