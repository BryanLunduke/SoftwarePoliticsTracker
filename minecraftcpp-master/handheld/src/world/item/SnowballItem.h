#ifndef NET_MINECRAFT_WORLD_ITEM__SnowballItem_H__
#define NET_MINECRAFT_WORLD_ITEM__SnowballItem_H__

//package net.minecraft.world.item;

#include "Item.h"
#include "../entity/player/Player.h"
#include "../entity/projectile/Snowball.h"
#include "../level/Level.h"

class SnowballItem: public Item {
    typedef Item super;
public:
    SnowballItem(int id)
    :   super(id)
    {
        maxStackSize = 16;
    }

    ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		if (!player->abilities.instabuild)
			instance->count--;

		level->playSound(player, "random.bow", 0.5f, 0.4f / (random.nextFloat() * 0.4f + 0.8f));
        if (!level->isClientSide) level->addEntity(new Snowball(level, player));
        return instance;
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__SnowballItem_H__*/
