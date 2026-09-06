#ifndef NET_MINECRAFT_WORLD_ITEM__ShearsItem_H__
#define NET_MINECRAFT_WORLD_ITEM__ShearsItem_H__

//package net.minecraft.world.item;

#include "Item.h"

#include "../entity/Mob.h"
#include "../level/tile/Tile.h"

// @todo: web and perhaps mineBlock
class ShearsItem: public Item
{
    typedef Item super;
public:
    ShearsItem(int itemId)
    :   super(itemId)
    {
        setMaxStackSize(1);
        setMaxDamage(238);
    }

    /*@Override*/
    bool mineBlock(ItemInstance* itemInstance, int tile, int x, int y, int z/*, Mob* owner*/) {
        if (tile == ((Tile*)Tile::leaves)->id || tile == Tile::web->id /*|| tile == Tile::tallgrass->id || tile == Tile::vine->id*/) {
            itemInstance->hurt(1);//, owner);
            return true;
        }
		return super::mineBlock(itemInstance, tile, x, y, z); // owner);
    }

    /*@Override*/
    bool canDestroySpecial(const Tile* tile) const {
        return tile->id == Tile::web->id;
    }

    /*@Override*/
    float getDestroySpeed(ItemInstance* itemInstance, Tile* tile) {
        if (tile->id == Tile::web->id || tile->id == ((Tile*)Tile::leaves)->id) {
            return 15;
        }
        if (tile->id == Tile::cloth->id) {
            return 5;
        }
        return super::getDestroySpeed(itemInstance, tile);
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ShearsItem_H__*/
