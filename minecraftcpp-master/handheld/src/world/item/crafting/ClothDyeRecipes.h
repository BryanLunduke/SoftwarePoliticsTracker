#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__ClothDyeRecipes_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__ClothDyeRecipes_H__

//package net.minecraft.world.item.crafting;

#include "Recipes.h"
#include "../DyePowderItem.h"
#include "../Item.h"
#include "../ItemInstance.h"
#include "../../level/tile/ClothTile.h"
#include "../../level/tile/Tile.h"

class ClothDyeRecipes
{
public:
    static void addRecipes(Recipes* r){
        // recipes for converting cloth to colored cloth using dye
        for (int i = 0; i < 16; i++) {
			if (i == DyePowderItem::BLACK
			 ||	i == DyePowderItem::BROWN
			 ||	i == DyePowderItem::SILVER
			 ||	i == DyePowderItem::GRAY
			 || i == DyePowderItem::WHITE)
				continue;

			r->addShapelessRecipe(ItemInstance(Tile::cloth, 1, ClothTile::getTileDataForItemAuxValue(i)),
				definition(0, ItemInstance(Item::dye_powder, 1, i), 0, ItemInstance(Item::items[Tile::cloth->id], 1, 0)));
        }
		// White cloth/wool-block from any cloth block (including white...)
		/*
		r->addShapelessRecipe(ItemInstance(Tile::cloth, 1, 15-DyePowderItem::WHITE),
			definition(0, ItemInstance(Tile::cloth, 1, Recipe::ANY_AUX_VALUE)));
			*/

		// some dye recipes
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::YELLOW),
			definition(0, Tile::flower));
        //r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::RED),
			//definition(Tile*)Tile::rose));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 3, DyePowderItem::WHITE),
			definition(0, Item::bone));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::PINK), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::ORANGE), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::YELLOW)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::LIME), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::GREEN), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
   //     r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::GRAY), //
   //         definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
   //     r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::SILVER), //
   //         definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::GRAY), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
   //     r->addShapelessRecipe(ItemInstance(Item::dye_powder, 3, DyePowderItem::SILVER), //
   //         definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
//        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::BROWN), //
//                definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::ORANGE)));
//        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 3, DyePowderItem::BROWN), //
//                definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLACK), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::YELLOW)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::LIGHT_BLUE), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::CYAN), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::GREEN)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::PURPLE), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED)));
        //r->addShapelessRecipe(ItemInstance(Item::dye_powder, 2, DyePowderItem::MAGENTA), //
        //    definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::PURPLE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::PINK)));
        //r->addShapelessRecipe(ItemInstance(Item::dye_powder, 3, DyePowderItem::MAGENTA), //
        //    definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::PINK)));
        r->addShapelessRecipe(ItemInstance(Item::dye_powder, 4, DyePowderItem::MAGENTA), //
            definition(0, ItemInstance(Item::dye_powder, 1, DyePowderItem::BLUE), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::RED), 0, ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE)));
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__ClothDyeRecipes_H__*/
