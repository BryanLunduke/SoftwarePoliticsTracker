#include "StonecutterScreen.h"
#include "CraftingFilters.h"
#include "../../../../world/level/material/Material.h"
#include "../../../../world/item/ItemCategory.h"

StonecutterScreen::StonecutterScreen()
:	super(Recipe::SIZE_3X3)
{
	setSingleCategoryAndIcon(ItemCategory::Structures, 5);
}

StonecutterScreen::~StonecutterScreen() {
}

bool StonecutterScreen::filterRecipe(const Recipe& r) {
	return CraftingFilters::isStonecutterItem(r.getResultItem());
}
