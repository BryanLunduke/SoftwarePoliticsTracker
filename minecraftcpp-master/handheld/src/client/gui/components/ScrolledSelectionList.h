#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrolledSelectionList_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrolledSelectionList_H__

#include "../GuiComponent.h"
class Minecraft;
class Tesselator;


class ScrolledSelectionList : public GuiComponent
{
	static const int NO_DRAG = -1;
	static const int DRAG_OUTSIDE = -2;
	static const int DRAG_NORMAL = 0;
	static const int DRAG_SKIP = 1; // special case to fix android jump bug
public:
	ScrolledSelectionList(Minecraft* _minecraft, int _width, int _height, int _y0, int _y1, int _itemHeight);

	virtual void setRenderSelection(bool _renderSelection);
protected:
	void setRenderHeader(bool _renderHeader, int _headerHeight);

	virtual int getNumberOfItems() = 0;

	virtual void selectItem(int item, bool doubleClick) = 0;
	virtual bool isSelectedItem(int item) = 0;

	virtual int getMaxPosition();

	virtual void renderItem(int i, int x, int y, int h, Tesselator& t) = 0;
	virtual void renderHeader(int x, int y, Tesselator& t) {}
	virtual void renderBackground() = 0;
	virtual void renderDecorations(int mouseX, int mouseY) {}

	virtual void clickedHeader(int headerMouseX, int headerMouseY) {}
public:
	virtual int getItemAtPosition(int x, int y);

	virtual void capYPosition();

	virtual void render(int xm, int ym, float a);
	virtual void renderHoleBackground(float y0, float y1, int a0, int a1);
	void renderDirtBackground();
protected:
	Minecraft* minecraft;

	float y0;
	float y1;
	int itemHeight;
private:
	int width;
	int height;
	float x1;
	float x0;

	int ignoreY; // new attempt to fix android jump bug
	int dragState;
	float yDrag;
	float yo;
	float yInertia;

	int selectionY;
	long lastSelectionTime;

	bool renderSelection;
	bool doRenderHeader;
	int headerHeight;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__ScrolledSelectionList_H__*/
