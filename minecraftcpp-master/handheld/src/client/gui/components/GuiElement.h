#ifndef NET_MINECRAFT_CLIENT_GUI__GuiElement_H__
#define NET_MINECRAFT_CLIENT_GUI__GuiElement_H__
#include "../GuiComponent.h"

class Tesselator;
class Minecraft;

class GuiElement : public GuiComponent {
public:
	GuiElement(bool active=false, bool visible=true, int x = 0, int y = 0, int width=24, int height=24);
    virtual ~GuiElement() {}
    virtual void tick(Minecraft* minecraft) {}
    virtual void render(Minecraft* minecraft, int xm, int ym) { }
	virtual void setupPositions() {}
	virtual void mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum) {}
	virtual void mouseReleased(Minecraft* minecraft, int x, int y, int buttonNum) {}
	virtual bool pointInside(int x, int y);
	void setVisible(bool visible);
	bool active;
	bool visible;
	int x;
	int y;
	int width;
	int height;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__GuiElement_H__*/
