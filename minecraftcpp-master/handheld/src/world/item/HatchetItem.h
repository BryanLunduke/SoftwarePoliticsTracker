#ifndef NET_MINECRAFT_WORLD_ITEM__HatchetItem_H__
#define NET_MINECRAFT_WORLD_ITEM__HatchetItem_H__

//package net.minecraft.world.item;

#include "DiggerItem.h"
#include <vector>

class Tile;
class Tier;
class Material;

class HatchetItem: public DiggerItem
{
    typedef DiggerItem super;
public:
    HatchetItem(int id, const Tier& tier);

    //@Override
    float getDestroySpeed(ItemInstance* itemInstance, Tile* tile);
};

#endif /*NET_MINECRAFT_WORLD_ITEM__HatchetItem_H__*/
