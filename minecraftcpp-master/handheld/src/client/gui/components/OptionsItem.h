#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsItem_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsItem_H__

#include <string>
#include <vector>
#include "GuiElementContainer.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../client/Options.h"
class Font; 
class Textures;
class NinePatchLayer;
class ItemPane;

class OptionsItem: public GuiElementContainer
{
	typedef GuiElementContainer super;
public:
	OptionsItem(std::string label, GuiElement* element);
	virtual void render(Minecraft* minecraft, int xm, int ym);
	void setupPositions();

private:
	std::string label;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__OptionsItem_H__*/
