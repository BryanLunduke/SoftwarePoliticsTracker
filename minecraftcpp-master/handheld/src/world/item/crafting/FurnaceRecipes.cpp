#include "FurnaceRecipes.h"
#include "../../level/tile/Tile.h"
#include "../DyePowderItem.h"
#include "../CoalItem.h"

/*static*/
FurnaceRecipes* FurnaceRecipes::instance = NULL;

const FurnaceRecipes* FurnaceRecipes::getInstance()
{
	if (!instance) instance = new FurnaceRecipes();
	return instance;
}

void FurnaceRecipes::addFurnaceRecipe( int itemId, const ItemInstance& result )
{
	recipes.insert(std::make_pair(itemId, result));
}

bool FurnaceRecipes::isFurnaceItem( int itemId ) const
{
	return recipes.find(itemId) != recipes.end();
}

ItemInstance FurnaceRecipes::getResult( int itemId ) const
{
	Map::const_iterator cit = recipes.find(itemId);
	return (cit != recipes.end())? cit->second : ItemInstance();
}

const FurnaceRecipes::Map& FurnaceRecipes::getRecipes() const
{
	return recipes;
}

FurnaceRecipes::FurnaceRecipes()
{
	addFurnaceRecipe(Tile::ironOre->id,     ItemInstance(Item::ironIngot));
	addFurnaceRecipe(Tile::goldOre->id,     ItemInstance(Item::goldIngot));
	addFurnaceRecipe(Tile::emeraldOre->id,  ItemInstance(Item::emerald));
	addFurnaceRecipe(Tile::sand->id,        ItemInstance(Tile::glass));
	addFurnaceRecipe(Item::porkChop_raw->id,ItemInstance(Item::porkChop_cooked));
	addFurnaceRecipe(Item::beef_raw->id,    ItemInstance(Item::beef_cooked));
	addFurnaceRecipe(Item::chicken_raw->id, ItemInstance(Item::chicken_cooked));
	//addFurnaceRecipe(Item::fish_raw->id,    ItemInstance(Item::fish_cooked));
	addFurnaceRecipe(Tile::stoneBrick->id,  ItemInstance(Tile::rock));
	addFurnaceRecipe(Item::clay->id,        ItemInstance(Item::brick));
	addFurnaceRecipe(Tile::cactus->id,      ItemInstance(Item::dye_powder, 1, DyePowderItem::GREEN));
	addFurnaceRecipe(Tile::mushroom2->id, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED));
	addFurnaceRecipe(Tile::treeTrunk->id,   ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL));
	addFurnaceRecipe(Tile::netherrack->id,  ItemInstance(Item::netherbrick));
	/*
		// special silk touch related recipes:
		addFurnaceRecipe(Tile::coalOre->id, ItemInstance(Item::coal));
		addFurnaceRecipe(Tile::redStoneOre->id, ItemInstance(Item::redStone));
		addFurnaceRecipe(Tile::lapisOre->id, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE));
	*/
}

void FurnaceRecipes::teardownFurnaceRecipes()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
}
