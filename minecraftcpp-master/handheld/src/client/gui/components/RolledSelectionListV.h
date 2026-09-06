#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListV_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListV_H__

#include "../GuiComponent.h"
class Minecraft;
class Tesselator;


class RolledSelectionListV : public GuiComponent
{
	static const int NO_DRAG		= -1;
	static const int DRAG_OUTSIDE	= -2;
	static const int DRAG_NORMAL	=  0;
public:
	RolledSelectionListV(Minecraft* minecraft, int width, int height, int x0, int x1, int y0, int y1, int itemHeight);

	virtual int getItemAtPosition(int x, int y);

	virtual bool capYPosition();

	virtual void tick();
	virtual void render(int xm, int ym, float a);
	virtual void renderHoleBackground(/*float x0, float x1,*/ float y0, float y1, int a0, int a1);
	virtual void setRenderSelection(bool _renderSelection);
	virtual void setComponentSelected(bool selected);
protected:
	void setRenderHeader(bool _renderHeader, int _headerHeight);

	virtual int getNumberOfItems() = 0;

	virtual void selectStart(int item) {}
	virtual void selectCancel() {}
	virtual void selectItem(int item, bool doubleClick) = 0;
	virtual bool isSelectedItem(int item) = 0;

	virtual int getMaxPosition();
	virtual float getPos(float alpha);
	virtual void touched();

	virtual void renderItem(int i, int x, int y, int h, Tesselator& t) = 0;
	virtual void renderHeader(int x, int y, Tesselator& t) {}
	virtual void renderBackground() = 0;
	virtual void renderForeground() {}
	virtual void renderDecorations(int mouseX, int mouseY) {}

	virtual void clickedHeader(int headerMouseX, int headerMouseY) {}
	virtual int convertSelection(int item, int xm, int ym) { return item; }

	int getItemAtYPositionRaw(int y);
	void evaluate(int xm, int ym);
	virtual void onPreRender();
	virtual void onPostRender();
	void renderDirtBackground();
protected:
	Minecraft* minecraft;

	float x0;
	float x1;
	int itemHeight;
	int width;
	int height;
//private:
	float y0;
	float y1;

	int dragState;
	float yDrag;
	float yo;
	float yoo;
	float yInertia;
	float _yinertia;

	int selectionY;
	bool renderSelection;
	bool _componentSelected;

	bool _renderDirtBackground;
	bool _renderTopBorder;
	bool _renderBottomBorder;

	int _lastxm;
	int _lastym;
private:
	int headerHeight;
	bool doRenderHeader;
	long lastSelectionTime;
	int lastSelection;

	float _lastyoo;

	float _stickPixels;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__RolledSelectionListV_H__*/
