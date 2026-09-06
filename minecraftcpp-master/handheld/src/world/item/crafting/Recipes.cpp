#include "Recipes.h"
#include "ShapedRecipe.h"
#include "ShapelessRecipe.h"

#include "ClothDyeRecipes.h"
#include "ToolRecipes.h"
#include "WeaponRecipes.h"
#include "StructureRecipes.h"
#include "FoodRecipes.h"
#include "FurnaceRecipes.h"
#include "ArmorRecipes.h"
#include "OreRecipes.h"
#include "../CoalItem.h"
#include "../../level/tile/StoneSlabTile.h"

/*static*/
Recipes* Recipes::instance = NULL;

Recipes::Recipes()
{
	ToolRecipes::addRecipes(this);
	WeaponRecipes::addRecipes(this);
	OreRecipes::addRecipes(this);
	FoodRecipes::addRecipes(this);
	StructureRecipes::addRecipes(this);
	ArmorRecipes::addRecipes(this);
	ClothDyeRecipes::addRecipes(this);

	addShapedRecipe(ItemInstance(Item::paper, 3), //
		"###", //

		definition('#', Item::reeds));

	addShapedRecipe(ItemInstance(Item::book, 1), //
		"#", //
		"#", //
		"#", //

		definition('#', Item::paper));

	addShapedRecipe(ItemInstance(Tile::fence, 2), //
		"###", //
		"###", //

		definition('#', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::netherFence, 6), //
	//	"###", //
	//	"###", //

	//	'#', Tile::netherBrick);

	addShapedRecipe(ItemInstance(Tile::fenceGate, 1), //
		"#W#", //
		"#W#", //

		definition('#', Item::stick, 'W', Tile::wood));

	//addShapedRecipe(ItemInstance(Tile::recordPlayer, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::wood, 'X', Item::emerald));

	//addShapedRecipe(ItemInstance(Tile::musicBlock, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::wood, 'X', Item::redStone));

	addShapedRecipe(ItemInstance(Tile::bookshelf, 1), //
		"###", //
		"XXX", //
		"###", //

		definition('#', Tile::wood, 'X', Item::book));

	addShapedRecipe(ItemInstance(Tile::snow, 1), //
		"##", //
		"##", //

		definition('#', Item::snowBall));

	addShapedRecipe(ItemInstance(Tile::clay, 1), //
		"##", //
		"##", //

		definition('#', Item::clay));

	addShapedRecipe(ItemInstance(Tile::redBrick, 1), //
		"##", //
		"##", //

		definition('#', Item::brick));

	addShapedRecipe(ItemInstance(Tile::lightGem, 1), //
		"##", //
		"##", //

		definition('#', Item::yellowDust));

	addShapedRecipe(ItemInstance(Tile::cloth, 1), //
		"##", //
		"##", //

		definition('#', Item::string));

	addShapedRecipe(ItemInstance(Tile::tnt, 1), //
		"X#X", //
		"#X#", //
		"X#X", //

		definition(	'X', Item::sulphur,
					'#', Tile::sand));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::COBBLESTONE_SLAB), //
		"###", //

		definition('#', Tile::stoneBrick));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::STONE_SLAB), //
		"###", //

		definition('#', Tile::rock));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::SAND_SLAB), //
		"###", //
		definition('#', Tile::sandStone));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::WOOD_SLAB), //
		"###", //

		definition('#', Tile::wood));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::BRICK_SLAB), //
		"###", //

		definition('#', Tile::redBrick));

	addShapedRecipe(ItemInstance(Tile::stoneSlabHalf, 6, StoneSlabTile::SMOOTHBRICK_SLAB), //
		"###", //
		definition('#', Tile::stoneBrickSmooth));

	addShapedRecipe(ItemInstance(Tile::ladder, 2), //
		"# #", //
		"###", //
		"# #", //

		definition('#', Item::stick));

	addShapedRecipe(ItemInstance(Item::door_wood, 1), //
		"##", //
		"##", //
		"##", //

		definition('#', Tile::wood));

	addShapedRecipe(ItemInstance(Tile::trapdoor, 2), //
		"###", //
		"###", //

		definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Item::door_iron, 1), //
	//	"##", //
	//	"##", //
	//	"##", //

	//	definition('#', Item::ironIngot));

	addShapedRecipe(ItemInstance(Item::sign, 1), //
		"###", //
		"###", //
		" X ", //

		definition('#', Tile::wood, 'X', Item::stick));

	//addShapedRecipe(ItemInstance(Item::cake, 1), //
	//	"AAA", //
	//	"BEB", //
	//	"CCC", //

	//	definition(	'A', Item::milk,//
	//				'B', Item::sugar,//
	//				'C', Item::wheat, 'E', Item::egg));

	addShapedRecipe(ItemInstance(Item::sugar, 1), //
		"#", //

		definition('#', Item::reeds));

	addShapedRecipe(ItemInstance(Tile::wood, 4), //
		"#", //

		definition('#', Tile::treeTrunk));

	addShapedRecipe(ItemInstance(Item::stick, 4), //
		"#", //
		"#", //

		definition('#', Tile::wood));

	addShapedRecipe(ItemInstance(Tile::torch, 4), //
		"X", //
		"#", //

		definition(	'X', Item::coal,//
					'#', Item::stick));
	// torch made of charcoal
	addShapedRecipe(ItemInstance(Tile::torch, 4), //
		"X", //
		"#", //

		definition( 'X', ItemInstance(Item::coal, 1, CoalItem::CHAR_COAL),//
					'#', Item::stick));

	addShapedRecipe(ItemInstance(Item::bowl, 4), //
		"# #", //
		" # ", //

		definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Item::glassBottle, 3), //
	//	"# #", //
	//	" # ", //

	//	'#', Tile::glass);

	//addShapedRecipe(ItemInstance(Tile::rail, 16), //
	//	"X X", //
	//	"X#X", //
	//	"X X", //

	//	definition(	'X', Item::ironIngot,//
	//				'#', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::goldenRail, 6), //
	//	"X X", //
	//	"X#X", //
	//	"XRX", //

	//	'X', Item::goldIngot,//
	//	'R', Item::redStone,//
	//	'#', Item::stick);

	//addShapedRecipe(ItemInstance(Tile::detectorRail, 6), //
	//	"X X", //
	//	"X#X", //
	//	"XRX", //

	//	'X', Item::ironIngot,//
	//	'R', Item::redStone,//
	//	'#', Tile::pressurePlate_stone);

	//addShapedRecipe(ItemInstance(Item::minecart, 1), //
	//	"# #", //
	//	"###", //

	//	definition('#', Item::ironIngot));

	//addShapedRecipe(ItemInstance(Item::cauldron, 1), //
	//	"# #", //
	//	"# #", //
	//	"###", //

	//	'#', Item::ironIngot);

	//addShapedRecipe(ItemInstance(Item::brewingStand, 1), //
	//	" B ", //
	//	"###", //

	//	'#', Tile::stoneBrick, 'B', Item::blazeRod);

	//addShapedRecipe(ItemInstance(Tile::litPumpkin, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::pumpkin, 'B', Tile::torch));

	//addShapedRecipe(ItemInstance(Item::minecart_chest, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::chest, 'B', Item::minecart));

	//addShapedRecipe(ItemInstance(Item::minecart_furnace, 1), //
	//	"A", //
	//	"B", //

	//	definition('A', Tile::furnace, 'B', Item::minecart));

	//addShapedRecipe(ItemInstance(Item::boat, 1), //
	//	"# #", //
	//	"###", //

	//	definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Item::bucket_empty, 1), //
	//	"# #", //
	//	" # ", //

	//	definition('#', Item::ironIngot));

	addShapedRecipe(ItemInstance(Item::flintAndSteel, 1), //
		"A ", //
		" B", //

		definition('A', Item::ironIngot, 'B', Item::flint));

	addShapedRecipe(ItemInstance(Item::bread, 1), //
		"###", //

		definition('#', Item::wheat));

	addShapedRecipe(ItemInstance(Tile::stairs_wood, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Item::fishingRod, 1), //
	//	"  #", //
	//	" #X", //
	//	"# X", //

	//	definition('#', Item::stick, 'X', Item::string));

	addShapedRecipe(ItemInstance(Tile::stairs_stone, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::stoneBrick));

	addShapedRecipe(ItemInstance(Tile::stairs_brick, 4), //
		"#  ", //
		"## ", //
		"###", //
		definition('#', Tile::redBrick));

	addShapedRecipe(ItemInstance(Tile::stairs_stoneBrickSmooth, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::stoneBrickSmooth));

	addShapedRecipe(ItemInstance(Tile::stairs_netherBricks, 4), //
		"#  ", //
		"## ", //
		"###", //

		definition('#', Tile::netherBrick));

	addShapedRecipe(ItemInstance(Item::painting, 1), //
		"###", //
		"#X#", //
		"###", //

		definition('#', Item::stick, 'X', Tile::cloth));

	//addShapedRecipe(ItemInstance(Item::apple_gold, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	definition('#', Tile::goldBlock, 'X', Item::apple));

	//addShapedRecipe(ItemInstance(Tile::lever, 1), //
	//	"X", //
	//	"#", //

	//	definition('#', Tile::stoneBrick, 'X', Item::stick));

	//addShapedRecipe(ItemInstance(Tile::notGate_on, 1), //
	//	"X", //
	//	"#", //

	//	definition('#', Item::stick, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::diode, 1), //
	//	"#X#", //
	//	"III", //

	//	definition('#', Tile::notGate_on, 'X', Item::redStone, 'I', Tile::rock));


	//addShapedRecipe(ItemInstance(Item::clock, 1), //
	//	" # ", //
	//	"#X#", //
	//	" # ", //

	//	definition('#', Item::goldIngot, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::compass, 1), //
	//	" # ", //
	//	"#X#", //
	//	" # ", //

	//	definition('#', Item::ironIngot, 'X', Item::redStone));

	//addShapedRecipe(ItemInstance(Item::map, 1), //
	//	"###", //
	//	"#X#", //
	//	"###", //

	//	'#', Item::paper, 'X', Item::compass);

	//addShapedRecipe(ItemInstance(Tile::button, 1), //
	//	"#", //
	//	"#", //

	//	definition('#', Tile::rock));


	//addShapedRecipe(ItemInstance(Tile::pressurePlate_stone, 1), //
	//	"##", //

	//	definition('#', Tile::rock));

	//addShapedRecipe(ItemInstance(Tile::pressurePlate_wood, 1), //
	//	"##", //

	//	definition('#', Tile::wood));

	//addShapedRecipe(ItemInstance(Tile::dispenser, 1), //
	//	"###", //
	//	"#X#", //
	//	"#R#", //

	//	definition('#', Tile::stoneBrick, 'X', Item::bow, 'R', Item::redStone));

	//addShapedRecipe(ItemInstance(Tile::pistonBase, 1), //
	//	"TTT", //
	//	"#X#", //
	//	"#R#", //

	//	'#', Tile::stoneBrick, 'X', Item::ironIngot, 'R', Item::redStone, 'T', Tile::wood);

	//addShapedRecipe(ItemInstance(Tile::pistonStickyBase, 1), //
	//	"S", //
	//	"P", //

	//	'S', Item::slimeBall, 'P', Tile::pistonBase);

	addShapedRecipe(ItemInstance(Item::bed, 1), //
		"###", //
		"XXX", //
		definition('#', Tile::cloth, 'X', Tile::wood));

	//addShapedRecipe(ItemInstance(Tile::enchantTable, 1), //
	//	" B ", //
	//	"D#D", //
	//	"###", //

	//	'#', Tile::obsidian, 'B', Item::book, 'D', Item::emerald);

	//addShapelessRecipe(ItemInstance(Item::eyeOfEnder, 1), //
	//	Item::enderPearl, Item::blazePowder);
	addShapedRecipe(ItemInstance(Tile::netherReactor, 1), //
		"X#X", //
		"X#X", //
		"X#X", //

		definition('#', Item::emerald, 'X', Item::ironIngot));

	LOGI("%d recipes\n", (int)recipes.size());
}

void Recipes::addShapedRecipe( const ItemInstance& result, const RowList& rows, const TypeList& types )
{
	if (rows.empty()) {
		LOGE("Recipes::addShapedRecipe: adding an empty recipe!\n");
		return;
	}

	std::string map = "";
	int width = rows[0].length();
	int height = rows.size();

	for (unsigned int i = 0; i < rows.size(); ++i)
		map += rows[i];

	typedef std::map<char, ItemInstance> Map;
	Map mappings;
	for (unsigned int i = 0; i < types.size(); i++) {
		const Type& type = types[i];
		char from = type.c;
		ItemInstance to;

		if (type.item) {
			to = ItemInstance(type.item);
		} else if (type.tile) {
			to = ItemInstance(type.tile, 1, Recipe::ANY_AUX_VALUE);
		} else if (!type.itemInstance.isNull()) {
			to = type.itemInstance;
		}
		
		mappings.insert(std::make_pair(from, to));
	}

	ItemInstance* ids = new ItemInstance[width * height];

	for (int i = 0; i < width * height; i++) {
		char ch = map[i];
		Map::iterator it = mappings.find(ch);
		if (it != mappings.end())
			ids[i] = it->second;
	}

	// <ids> are deleted in ShapedRecipe
	recipes.push_back(new ShapedRecipe(width, height, ids, result));
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const TypeList& types) {
	addShapedRecipe(result, Shape(r0), types);
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const std::string& r1, const TypeList& types) {
	addShapedRecipe(result, Shape(r0, r1), types);
}

void Recipes::addShapedRecipe( const ItemInstance& result, const std::string& r0, const std::string& r1, const std::string& r2, const TypeList& types) {
	addShapedRecipe(result, Shape(r0, r1, r2), types);
}

void Recipes::addShapelessRecipe(const ItemInstance& result, const TypeList& types) {

    std::vector<ItemInstance> ingredients;

	for (unsigned int i = 0; i < types.size(); i++) {
		const Type& type = types[i];
		if (type.item) {
			ingredients.push_back( ItemInstance(type.item) );
		} else if (type.tile) {
			ingredients.push_back( ItemInstance(type.tile) );
		} else if (!type.itemInstance.isNull()) {
			ingredients.push_back( type.itemInstance );
		} else {
			LOGE("addShapeLessRecipe: Incorrect shapeless recipe!\n");
		}
    }

    recipes.push_back(new ShapelessRecipe(result, ingredients));
}

RowList Recipes::Shape( const std::string& r0 ) {
	RowList rows;
	rows.push_back(r0);
	return rows;
}

RowList Recipes::Shape( const std::string& r0, const std::string& r1 ) {
	RowList rows;
	rows.push_back(r0);
	rows.push_back(r1);
	return rows;
}

RowList Recipes::Shape( const std::string& r0, const std::string& r1, const std::string& r2 ) {
	RowList rows;
	rows.push_back(r0);
	rows.push_back(r1);
	rows.push_back(r2);
	return rows;
}

Recipe* Recipes::getRecipeFor( const ItemInstance& result )
{
	for (unsigned int i = 0; i < recipes.size(); ++i) {
		Recipe* recipe = recipes[i];
		ItemInstance res = recipe->getResultItem();
		if (result.id != res.id) continue;

		if ( result.count == 0 && result.getAuxValue() == res.getAuxValue()
		||	(result.count == res.count && result.getAuxValue() == res.getAuxValue()))
			return recipe;
	}
	return NULL;
}

Recipes* Recipes::getInstance()
{
	if (!instance)
		instance = new Recipes();

	return instance;
}

void Recipes::teardownRecipes()
{
	if (instance) {
		delete instance;
		instance = NULL;
	}
	FurnaceRecipes::teardownFurnaceRecipes();
}

const RecipeList& Recipes::getRecipes()
{
	return recipes;
}

ItemInstance Recipes::getItemFor( CraftingContainer* craftSlots )
{
	int count = 0;
	ItemInstance* first;
	ItemInstance* second;
	for (int i = 0; i < craftSlots->getContainerSize(); i++) {
		ItemInstance* item = craftSlots->getItem(i);
		if (item) {
			if (count == 0) first = item;
			if (count == 1) second = item;
			count++;
		}
	}

	if (count == 2 && first->id == second->id && first->count == 1 && second->count == 1 && Item::items[first->id]->canBeDepleted()) {
		Item* item = Item::items[first->id];
		int remaining1 = item->getMaxDamage() - first->getDamageValue();
		int remaining2 = item->getMaxDamage() - second->getDamageValue();
		int remaining = (remaining1 + remaining2) + item->getMaxDamage() * 10 / 100;
		int resultDamage = item->getMaxDamage() - remaining;
		if (resultDamage < 0) resultDamage = 0;
		return ItemInstance(first->id, 1, resultDamage);
	}

	for (unsigned int i = 0; i < recipes.size(); i++) {
		Recipe* r = recipes[i];
		if (r->matches(craftSlots)) return r->assemble(craftSlots);
	}
	return ItemInstance();
}

Recipes::~Recipes()
{
	for (unsigned int i = 0; i < recipes.size(); ++i)
		delete recipes[i];
}
