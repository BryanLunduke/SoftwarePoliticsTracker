#ifndef NET_MINECRAFT_WORLD_ITEM__BowlFoodItem_H__
#define NET_MINECRAFT_WORLD_ITEM__BowlFoodItem_H__

//package net.minecraft.world.item;

#include "FoodItem.h"

class BowlFoodItem: public FoodItem
{
	typedef FoodItem super;
public:
	BowlFoodItem(int id, int nutrition)
	:   super(id, nutrition, false)
	{
		setMaxStackSize(1);
	}

	ItemInstance useTimeDepleted(ItemInstance* instance, Level* level, Player* player) {
		super::useTimeDepleted(instance, level, player);
		*instance = ItemInstance(Item::bowl);
		return *instance;
	}
};

#endif /*NET_MINECRAFT_WORLD_ITEM__BowlFoodItem_H__*/
