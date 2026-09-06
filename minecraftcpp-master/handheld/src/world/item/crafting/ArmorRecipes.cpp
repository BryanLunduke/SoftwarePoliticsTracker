#include "ArmorRecipes.h"
#include "Recipes.h"
#include "../../level/tile/Tile.h"

static RowList shapes[] = {
	// Helmet
	Recipes::Shape(
	"XXX", //
	"X X"),//

	// Chest plate
	Recipes::Shape(
	"X X",//
	"XXX",//
	"XXX"),//

	// Leggings
	Recipes::Shape(
	"XXX",//
	"X X",//
	"X X"),//

	// Boots
	Recipes::Shape(
	"X X",//
	"X X")//
};

void ArmorRecipes::addRecipes( Recipes* r )
{
	int materialIds[] = {Item::leather->id, /*((Tile*)Tile::fire)->id,*/ Item::ironIngot->id, Item::emerald->id, Item::goldIngot->id};

	const int NumMaterials = sizeof(materialIds) / sizeof(int);
	const int NumRecipes = sizeof(shapes) / sizeof(RowList);

	Item* map[NumRecipes][NumMaterials] = {
		{Item::helmet_cloth, /*Item::helmet_chain,*/ Item::helmet_iron, Item::helmet_diamond, Item::helmet_gold},
		{Item::chestplate_cloth, /*Item::chestplate_chain,*/ Item::chestplate_iron, Item::chestplate_diamond, Item::chestplate_gold}, 
		{Item::leggings_cloth, /*Item::leggings_chain,*/ Item::leggings_iron, Item::leggings_diamond, Item::leggings_gold}, 
		{Item::boots_cloth, /*Item::boots_chain,*/ Item::boots_iron, Item::boots_diamond, Item::boots_gold},
	};

	//const int OVERRIDDEN_MaterialCount = 2;
	for (int m = 0; m < NumMaterials; m++) {
		int materialId = materialIds[m];
		for (int t = 0; t < NumRecipes; t++) {
			Item* target = (Item*) map[t][m];

			r->addShapedRecipe( ItemInstance(target), shapes[t],
				definition('X', Item::items[materialId]) );
		}
	}
}
