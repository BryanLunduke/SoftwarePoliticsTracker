#ifndef NET_MINECRAFT_WORLD_ITEM__ClothTileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__ClothTileItem_H__

//package net.minecraft.world.item;

#include "TileItem.h"
#include "DyePowderItem.h"
#include "../level/tile/ClothTile.h"

class ClothTileItem: public TileItem
{
    typedef TileItem super;
public:
    ClothTileItem(int id)
    :   super(id)
    {
        setMaxDamage(0);
        setStackedByData(true);
    }

    /*@Override*/
    int getIcon(int itemAuxValue) {
        return Tile::cloth->getTexture(2, ClothTile::getTileDataForItemAuxValue(itemAuxValue));
    }

    /*@Override*/
    int getLevelDataForAuxValue(int auxValue) {
        return auxValue;
    }

    /*@Override*/
    std::string getDescriptionId(const ItemInstance* instance) const {
        return super::getDescriptionId() + "." + DyePowderItem::COLOR_DESCS[ClothTile::getTileDataForItemAuxValue(instance->getAuxValue())];
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ClothTileItem_H__*/
