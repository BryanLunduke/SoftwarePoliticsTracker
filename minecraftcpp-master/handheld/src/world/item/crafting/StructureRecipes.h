#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__StructureRecipes_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__StructureRecipes_H__

//package net.minecraft.world.item.crafting;

/* import net.minecraft.world.item.* */
#include "Recipes.h"
#include "../../level/tile/Tile.h"
#include "../../level/tile/SandStoneTile.h"
#include "../../level/tile/StoneSlabTile.h"

class StructureRecipes
{
public:
    static void addRecipes(Recipes* r) {
        r->addShapedRecipe(ItemInstance(Tile::chest), //
            "###", //
            "# #", //
            "###", //

            definition('#', Tile::wood));

        r->addShapedRecipe(ItemInstance(Tile::furnace), //
			"###", //
			"# #", //
			"###", //

			definition('#', Tile::stoneBrick));

        r->addShapedRecipe(ItemInstance(Tile::workBench), //
            "##", //
            "##", //

            definition('#', Tile::wood));

		r->addShapedRecipe(ItemInstance(Tile::stonecutterBench), //
			"##", //
			"##", //

			definition('#', Tile::stoneBrick));


        r->addShapedRecipe(ItemInstance(Tile::sandStone), //
            "##", //
            "##", //

            definition('#', Tile::sand));

		r->addShapedRecipe(ItemInstance(Tile::sandStone, 4, SandStoneTile::TYPE_SMOOTHSIDE), //
			"##", //
			"##", //

			definition('#', Tile::sandStone));

		r->addShapedRecipe(ItemInstance(Tile::sandStone, 1, SandStoneTile::TYPE_HEIROGLYPHS), //
			"#", //
			"#", //

			definition('#', ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SAND_SLAB)));

        r->addShapedRecipe(ItemInstance(Tile::stoneBrickSmooth, 4), //
            "##", //
            "##", //

            definition('#', Tile::rock));

        //r->addShapedRecipe(ItemInstance(Tile::ironFence, 16), //
        //    "###", //
        //    "###", //

        //    definition('#', Item::ironIngot));

        r->addShapedRecipe(ItemInstance(Tile::thinGlass, 16), //
            "###", //
            "###", //

            definition('#', Tile::glass));

		r->addShapedRecipe(ItemInstance(Tile::netherBrick, 1), //
			"NN", //
			"NN", //

			definition('N', Item::netherbrick));

		r->addShapedRecipe(ItemInstance(Tile::quartzBlock, 1), //
			"NN", //
			"NN", //

			definition('N', Item::netherQuartz));

        //r->addShapedRecipe(ItemInstance(Tile::redstoneLight, 1), //
        //    " R ", //
        //    "RGR", //
        //    " R ", //

        //    definition('R', Item::redStone, 'G', Tile::lightGem));
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__StructureRecipes_H__*/
