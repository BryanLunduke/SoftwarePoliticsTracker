#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListH_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListH_H__

#include "../GuiComponent.h"
class Minecraft;
class Tesselator;


class RolledSelectionListH : public GuiComponent
{
	static const int NO_DRAG		= -1;
	static const int DRAG_OUTSIDE	= -2;
	static const int DRAG_NORMAL	=  0;
public:
	RolledSelectionListH(Minecraft* minecraft, int width, int height, int x0, int x1, int y0, int y1, int itemWidth);

	virtual int getItemAtPosition(int x, int y);

	virtual bool capXPosition();

	virtual void tick();
	virtual void render(int xm, int ym, float a);
	virtual void renderHoleBackground(/*float x0, float x1,*/ float y0, float y1, int a0, int a1);
	virtual void setRenderSelection(bool _renderSelection);
	virtual void setComponentSelected(bool selected);
protected:
	void setRenderHeader(bool _renderHeader, int _headerHeight);

	virtual int getNumberOfItems() = 0;

	virtual void selectStart(int item, int localX, int localY) {}
	virtual void selectCancel() {}
	virtual void selectItem(int item, bool doubleClick) = 0;
	virtual bool isSelectedItem(int item) = 0;

	virtual int getMaxPosition();
	virtual float getPos(float alpha);
	virtual void touched();

	virtual void renderItem(int i, int x, int y, int h, Tesselator& t) = 0;
	virtual void renderHeader(int x, int y, Tesselator& t) {}
	virtual void renderBackground() = 0;
	virtual void renderDecorations(int mouseX, int mouseY) {}

	virtual void clickedHeader(int headerMouseX, int headerMouseY) {}
	int getItemAtXPositionRaw(int x);
protected:
	Minecraft* minecraft;

	float x0;
	float x1;
	int itemWidth;
	int width;
	int height;
//private:
	float y0;
	float y1;

	int dragState;
	float xDrag;
	float xo;
	float xoo;
	float xInertia;
	float _xinertia;

	int selectionX;
	bool renderSelection;
	bool _componentSelected;

	bool _renderTopBorder;
	bool _renderBottomBorder;

private:
	int headerWidth;
	bool doRenderHeader;
	long lastSelectionTime;
	int lastSelection;

	float _lastxoo;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListH_H__*/
