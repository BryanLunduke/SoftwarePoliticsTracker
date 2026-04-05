#include "WorkbenchScreen.h"
#include "CraftingFilters.h"
#include "../../../../world/level/material/Material.h"

WorkbenchScreen::WorkbenchScreen(int craftingSize)
:	super(craftingSize)
{
}

WorkbenchScreen::~WorkbenchScreen() {
}

bool WorkbenchScreen::filterRecipe(const Recipe& r) {
	return !CraftingFilters::isStonecutterItem(r.getResultItem());
}
