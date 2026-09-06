#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_WorkbenchScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_WorkbenchScreen_H__

#include "PaneCraftingScreen.h"

class WorkbenchScreen: public PaneCraftingScreen
{
	typedef PaneCraftingScreen super;
public:
    WorkbenchScreen(int craftingSize);
	~WorkbenchScreen();

private:
	bool filterRecipe(const Recipe& r);
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_WorkbenchScreen_H__*/
