#ifndef NET_MINECRAFT_WORLD_ITEM__BowItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BowItem_H__

//package net.minecraft.world.item;

#include "Item.h"

#include "../entity/player/Player.h"
#include "../entity/projectile/Arrow.h"
#include "../level/Level.h"
#include "../entity/player/Inventory.h"

class BowItem: public Item
{
    typedef Item super;
public:
    static const int MAX_DRAW_DURATION = 20 * 1;

    BowItem(int id)
    :   super(id)
    {
        maxStackSize = 1;
        this->setMaxDamage(384);
    }
    void releaseUsing( ItemInstance* itemInstance, Level* level, Player* player, int durationLeft ) {
		int timeHeld = getUseDuration(itemInstance) - durationLeft;
		float pow = timeHeld / (float) MAX_DRAW_DURATION;
		pow = ((pow * pow) + pow * 2) / 3;
		if (pow < 0.1) return;
		if (pow > 1) pow = 1;


		itemInstance->hurt(1);
		level->playSound(player, "random.bow", 1.0f, 1.0f / (random.nextFloat() * 0.4f + 1.2f) + pow * 0.5f);
		player->inventory->removeResource(Item::arrow->id);
		if (!level->isClientSide) {
			Arrow* arrow =	new Arrow(level, player, pow * 2.0f);
			if (pow == 1)
				arrow->critArrow = true;
			level->addEntity(arrow);
		}
    }

    int getUseDuration(ItemInstance* itemInstance) {
        return 20 * 60 * 60;
    }

    ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
		if(player->abilities.instabuild || player->hasResource(Item::arrow->id)) {
			player->startUsingItem(*instance, getUseDuration(instance));
		}
        return instance;
    }
	UseAnim::UseAnimation getUseAnimation() {return UseAnim::bow;}

    // /*public*/ ItemInstance use(ItemInstance instance, Level* level, Player* player) {
//            level->playSound(player, "random.bow", 1.0f, 1 / (random.nextFloat() * 0.4f + 0.8f));
//            if (!level->isOnline) level->addEntity(/*new*/ Arrow(level, player));
//        }
//        return instance;
//    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__BowItem_H__*/
