#ifndef NET_MINECRAFT_WORLD_ITEM__TreeTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TreeTileItem_H__

//package net.minecraft.world.item;

#include "../level/tile/Tile.h"
#include "ItemInstance.h"

class TreeTileItem: public TileItem
{
	typedef TileItem super;

public:
    TreeTileItem(int id)
    :	super(id)
	{
        setMaxDamage(0);
        setStackedByData(true);
    }

    int getIcon(const ItemInstance* itemInstance) {
        return Tile::treeTrunk->getTexture(2, itemInstance->getAuxValue());
    }

    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__TreeTileItem_H__*/
