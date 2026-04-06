#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__FurnaceRecipes_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__FurnaceRecipes_H__

//package net.minecraft.world.item.crafting;

#include "../ItemInstance.h"
#include <map>

class FurnaceRecipes
{
public:
    typedef std::map<int, ItemInstance> Map;

	static void teardownFurnaceRecipes();
    static const FurnaceRecipes* getInstance();

    bool isFurnaceItem(int itemId) const;

	ItemInstance getResult(int itemId) const;

    const Map& getRecipes() const;

private:
    FurnaceRecipes();

	void addFurnaceRecipe(int itemId, const ItemInstance& result);

    static FurnaceRecipes* instance;

	Map recipes;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__FurnaceRecipes_H__*/
