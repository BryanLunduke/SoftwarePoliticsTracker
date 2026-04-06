#ifndef NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__
#define NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__

//package net.minecraft.world.item.crafting;

#include "../../inventory/CraftingContainer.h"
#include "../ItemInstance.h"
#include "Recipe.h"

class ShapedRecipe: public Recipe
{
public:
    ShapedRecipe(int width, int height, ItemInstance* recipeItems, const ItemInstance& result)
	:	width(width),
		height(height),
		recipeItems(recipeItems),
		resultId(result.id),
		result(result)
		//@todo: move slots to upper left
	{
		for (int i = 0; i < width * height; ++i)
			if (!recipeItems[i].isNull())
				myItems.add( ItemPack::getIdForItemInstance(&recipeItems[i]));
    }

	~ShapedRecipe() {
		delete[] recipeItems;
	}

	int getMaxCraftCount(ItemPack& fromItems) {

		int count = fromItems.getMaxMultipliesOf(myItems);
		return count;


		//return (int)(Mth::random() * Mth::random() * 5);
	}

    ItemInstance getResultItem() const {
        return result;
    }

    bool matches(CraftingContainer* craftSlots) {
        for (int xOffs = 0; xOffs <= (3 - width); xOffs++) {
            for (int yOffs = 0; yOffs <= (3 - height); yOffs++) {
                if (matches(craftSlots, xOffs, yOffs, true)) return true;
                if (matches(craftSlots, xOffs, yOffs, false)) return true;
            }
        }
        return false;
    }

	ItemInstance assemble(CraftingContainer* craftSlots) {
		return ItemInstance(result.id, result.count, result.getAuxValue());
	}

	int size() {
		return width * height;
	}

	virtual std::vector<ItemInstance> getItems() {
		std::vector<ItemInstance> out;
		return out;
	}

private:
    bool matches(CraftingContainer* craftSlots, int xOffs, int yOffs, bool xFlip) {
        for (int x = 0; x < 3; x++) {
            for (int y = 0; y < 3; y++) {
                int xs = x - xOffs;
                int ys = y - yOffs;
                ItemInstance expected;
                if (xs >= 0 && ys >= 0 && xs < width && ys < height) {
                    if (xFlip) expected = recipeItems[(width - xs - 1) + ys * width];
                    else expected = recipeItems[xs + ys * width];
                }
                ItemInstance* item = craftSlots->getItem(x, y);
                if (!item && expected.isNull())
                    continue;
                if ((item == NULL) ^ expected.isNull())
                    return false;
                if (expected.id != item->id)
                    return false;
                if (expected.getAuxValue() != Recipe::ANY_AUX_VALUE && expected.getAuxValue() != item->getAuxValue())
                    return false;
            }
        }
        return true;
    }

	int getCraftingSize() {
		return (width <= 2 && height <= 2)? SIZE_2X2 : SIZE_3X3;
	}


public:
	const int resultId;
private:
	int width, height;
	ItemInstance* recipeItems;
	ItemInstance result;
	//ItemPack myItems;
};

#endif /*NET_MINECRAFT_WORLD_ITEM_CRAFTING__ShapedRecipe_H__*/
