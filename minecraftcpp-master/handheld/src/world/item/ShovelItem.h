#ifndef NET_MINECRAFT_WORLD_ITEM__ShovelItem_H__
#define NET_MINECRAFT_WORLD_ITEM__ShovelItem_H__

//package net.minecraft.world.item;

#include "DiggerItem.h"
#include <vector>

class Tile;
class Tier;

class ShovelItem: public DiggerItem
{
    typedef DiggerItem super;
public:
    ShovelItem(int id, const Tier& tier);

    bool canDestroySpecial(const Tile* tile) const;
};

#endif /*NET_MINECRAFT_WORLD_ITEM__ShovelItem_H__*/
