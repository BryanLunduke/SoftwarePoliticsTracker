#ifndef NET_MINECRAFT_WORLD_ITEM__PickaxeItem_H__
#define NET_MINECRAFT_WORLD_ITEM__PickaxeItem_H__

//package net.minecraft.world.item;

#include "DiggerItem.h"

class Tile;
class Tier;
class ItemInstance;

class PickaxeItem: public DiggerItem
{
    typedef DiggerItem super;
public:
    PickaxeItem(int id, const Tier& tier);

    bool canDestroySpecial(const Tile* tile) const;
    //@Override
    float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};

#endif /*NET_MINECRAFT_WORLD_ITEM__PickaxeItem_H__*/
