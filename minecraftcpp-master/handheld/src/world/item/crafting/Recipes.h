#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipes_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipes_H__

//package net.minecraft.world.item.crafting;

#include "../Item.h"
#include "../../inventory/CraftingContainer.h"
#include "../../level/tile/Tile.h"

#include "Recipe.h"

typedef std::vector<std::string> RowList;
typedef std::vector<Recipe*> RecipeList;

class Recipes
{
public:
    static Recipes* getInstance();
	const RecipeList& getRecipes();
	static void		teardownRecipes();

	class Type {
	public:
		Type(char c, Item* item)
			:	c(c), item(item), tile(NULL) {}
		Type(char c, Tile* tile)
			:	c(c), item(NULL), tile(tile) {}
		Type(char c, const ItemInstance& itemInstance)
			:	c(c), item(NULL), tile(NULL), itemInstance(itemInstance) {}
		bool isItem() { return item != NULL; }
		Item* item;
		Tile* tile;
		ItemInstance itemInstance;
		char c;
	} ;//Type;

	typedef std::vector<Type> TypeList;

	static RowList Shape(const std::string& r0);
	static RowList Shape(const std::string& r0, const std::string& r1);
	static RowList Shape(const std::string& r0, const std::string& r1, const std::string& r2);

	void addShapedRecipe(const ItemInstance& result, const std::string& r0, const TypeList& types);
	void addShapedRecipe(const ItemInstance& result, const std::string& r0, const std::string&, const TypeList& types);
	void addShapedRecipe(const ItemInstance& result, const std::string& r0, const std::string& r1, const std::string& r2, const TypeList& types);

	void addShapedRecipe(const ItemInstance& result, const RowList& rows, const TypeList& types);

	void addShapelessRecipe(const ItemInstance& result, const TypeList& types);

	Recipe* getRecipeFor(const ItemInstance& result);
private:
    Recipes();
	~Recipes();

    ItemInstance getItemFor(CraftingContainer* craftSlots);
private:
	static Recipes* instance;
	RecipeList recipes;
};


template <class T>
Recipes::TypeList definition(char c0, T t) {
	Recipes::TypeList list;
	list.push_back(Recipes::Type(c0, t));
	return list;
}

template <class T, class U>
Recipes::TypeList definition(char c0, T t, char c1, U u) {
	Recipes::TypeList list;
	list.push_back(Recipes::Type(c0, t));
	list.push_back(Recipes::Type(c1, u));
	return list;
}

template <class T, class U, class V>
Recipes::TypeList definition(char c0, T t, char c1, U u, char c2, V v) {
	Recipes::TypeList list;
	list.push_back(Recipes::Type(c0, t));
	list.push_back(Recipes::Type(c1, u));
	list.push_back(Recipes::Type(c2, v));
	return list;
}

template <class T, class U, class V, class W>
Recipes::TypeList definition(char c0, T t, char c1, U u, char c2, V v, char c3, W w) {
	Recipes::TypeList list;
	list.push_back(Recipes::Type(c0, t));
	list.push_back(Recipes::Type(c1, u));
	list.push_back(Recipes::Type(c2, v));
	list.push_back(Recipes::Type(c3, w));
	return list;
}


#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__Recipes_H__*/
