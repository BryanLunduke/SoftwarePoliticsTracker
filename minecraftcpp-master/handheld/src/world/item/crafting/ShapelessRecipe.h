#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapelessRecipe_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapelessRecipe_H__

//package net.minecraft.world.item.crafting;

#include "Recipe.h"
#include "../ItemInstance.h"
#include "../../inventory/CraftingContainer.h"

class ShapelessRecipe: public Recipe
{
	typedef std::vector<ItemInstance> Ingredients;
public:
    ShapelessRecipe(ItemInstance result, const Ingredients& ingredients)
	:	result(result),
		ingredients(ingredients)
	{
		for (unsigned int i = 0; i < ingredients.size(); ++i)
			if (!ingredients[i].isNull())
				myItems.add( ItemPack::getIdForItemInstance(&ingredients[i]) );
    }

    ItemInstance getResultItem() const {
        return result;
    }

	int getMaxCraftCount(ItemPack& fromItems) {
		int count = fromItems.getMaxMultipliesOf(myItems);
		return count;
		//return (int)(Mth::random() * Mth::random() * 5);
	}

    bool matches(CraftingContainer* craftSlots) {

        Ingredients tempList = ingredients;

        for (int y = 0; y < 3; y++) {
            for (int x = 0; x < 3; x++) {
                ItemInstance* item = craftSlots->getItem(x, y);

                if (item) {
                    bool found = false;
					for (unsigned int i = 0; i < ingredients.size(); ++i) {
						const ItemInstance& ingredient = ingredients[i];
                        if (item->id == ingredient.id && (ingredient.getAuxValue() == Recipe::ANY_AUX_VALUE || item->getAuxValue() == ingredient.getAuxValue())) {
                            found = true;

							Ingredients::iterator it = std::find(tempList.begin(), tempList.end(), ingredient);
							if (it != tempList.end())
								tempList.erase(it);

                            break;
                        }
                    }
                    if (!found) {
                        return false;
                    }
                }
            }
        }

        return tempList.empty();
    }

    ItemInstance assemble(CraftingContainer* craftSlots) {
        return result;
    }

    int size() {
        return (int)ingredients.size();
    }

	int getCraftingSize() {
		return (ingredients.size() > 4)? SIZE_3X3 : SIZE_2X2;
	}

private:
	const ItemInstance result;
	const Ingredients ingredients;
	//ItemPack myItems;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapelessRecipe_H__*/
