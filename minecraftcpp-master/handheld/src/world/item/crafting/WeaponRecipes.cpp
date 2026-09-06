#include "WeaponRecipes.h"

static RowList shapes[] = {
	Recipes::Shape(	"X", //
					"X",//
					"#"),//
};

void WeaponRecipes::addRecipes( Recipes* r )
{
	int materialIds[] = {Tile::wood->id, Tile::stoneBrick->id, Item::ironIngot->id, Item::emerald->id, Item::goldIngot->id};
	const int NumMaterials = sizeof(materialIds) / sizeof(int);
	const int NumRecipes = sizeof(shapes) / sizeof(RowList);

	Item* map[NumRecipes][NumMaterials] = {
		{Item::sword_wood, Item::sword_stone, Item::sword_iron, Item::sword_emerald, Item::sword_gold}, 
	};

	//const int OVERRIDDEN_MaterialCount = 2;
	for (int m = 0; m < NumMaterials; m++) {
		int mId = materialIds[m];
		for (int t = 0; t < NumRecipes; t++) {
			Item* target = (Item*) map[t][m];

			if (mId < 256) { // Tile
				r->addShapedRecipe( ItemInstance(target), shapes[t],
					definition('#', Item::stick, 'X', Tile::tiles[mId]) );
			} else { // Item
				r->addShapedRecipe( ItemInstance(target), shapes[t],
					definition('#', Item::stick, 'X', Item::items[mId]) );
			}
		}
	}

	r->addShapedRecipe(ItemInstance(Item::bow, 1), //
		" #X", //
		"# X", //
		" #X", //
		definition(	'X', Item::string,// 
					'#', Item::stick));

	r->addShapedRecipe(ItemInstance(Item::arrow, 4), //
		"X", //
		"#", //
		"Y", //
		definition(	'Y', Item::feather,// 
					'X', Item::flint,// 
					'#', Item::stick));
}
