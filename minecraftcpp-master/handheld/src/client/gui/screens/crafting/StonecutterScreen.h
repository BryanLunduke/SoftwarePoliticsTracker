#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_StonecutterScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_StonecutterScreen_H__

#include "PaneCraftingScreen.h"

class StonecutterScreen: public PaneCraftingScreen
{
	typedef PaneCraftingScreen super;
public:
    StonecutterScreen();
	~StonecutterScreen();

private:
	bool filterRecipe(const Recipe& r);
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_StonecutterScreen_H__*/
