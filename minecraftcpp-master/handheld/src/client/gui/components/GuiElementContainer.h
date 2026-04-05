#ifndef NET_MINECRAFT_CLIENT_GUI__GuiElementContainer_H__
#define NET_MINECRAFT_CLIENT_GUI__GuiElementContainer_H__
#include "GuiElement.h"
#include <vector>
class Tesselator;
class Minecraft;

class GuiElementContainer : public GuiElement {
public:
	GuiElementContainer(bool active=false, bool visible=true, int x = 0, int y = 0, int width=24, int height=24);
    virtual ~GuiElementContainer();
    virtual void render(Minecraft* minecraft, int xm, int ym);
	virtual void setupPositions();
	virtual void addChild(GuiElement* element);
	virtual void removeChild(GuiElement* element);

	virtual void tick( Minecraft* minecraft );

	virtual void mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum );

	virtual void mouseReleased( Minecraft* minecraft, int x, int y, int buttonNum );

protected:
	std::vector<GuiElement*> children;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__GuiElementContainer_H__*/
