#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__FoodRecipes_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__FoodRecipes_H__

//package net.minecraft.world.item.crafting;

#include "Recipes.h"
#include "../Item.h"
#include "../ItemInstance.h"
#include "../DyePowderItem.h"
#include "../../level/tile/Tile.h"

class FoodRecipes {
public:
    static void addRecipes(Recipes* r) {
        r->addShapelessRecipe(ItemInstance(Item::mushroomStew), //
                definition(0, Tile::mushroom1, 0, Tile::mushroom2, 0, Item::bowl));

        //r->addShapedRecipe(/*new*/ ItemInstance(Item::cookie, 8), //
        //        "#X#", //

        //        definition('X', /*new*/ ItemInstance(Item::dye_powder, 1, DyePowderItem::BROWN), '#', Item::wheat));

        r->addShapedRecipe(/*new*/ ItemInstance(Tile::melon), //
                "MMM", //
                "MMM", //
                "MMM", //

                definition('M', Item::melon));

        r->addShapedRecipe(/*new*/ ItemInstance(Item::seeds_melon), //
                "M", //

                definition('M', Item::melon));

        //r->addShapedRecipe(/*new*/ ItemInstance(Item::seeds_pumpkin, 4), //
        //        "M", //

        //        definition('M', Tile::pumpkin));

        //r->addShapelessRecipe(/*new*/ ItemInstance(Item::fermentedSpiderEye), //
        //        definition(0,Item::spiderEye, 0,Tile::mushroom1, 0,Item::sugar));

        //r->addShapelessRecipe(/*new*/ ItemInstance(Item::speckledMelon), //
        //        definition(0,Item::melon, 0,Item::goldNugget));

        //r->addShapelessRecipe(/*new*/ ItemInstance(Item::blazePowder, 2), //
        //        definition(0,Item::blazeRod));

        //r->addShapelessRecipe(/*new*/ ItemInstance(Item::magmaCream), //
        //        definition(0,Item::blazePowder, 0,Item::slimeBall));
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__FoodRecipes_H__*/
