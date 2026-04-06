#ifndef NET_MINECRAFT_CLIENT_GUI_COMPONENTS__InventoryPane_H__
#define NET_MINECRAFT_CLIENT_GUI_COMPONENTS__InventoryPane_H__

#include "ScrollingPane.h"
#include "ImageButton.h"

class Minecraft;
class ItemInstance;
class Font;
class IArea;

namespace Touch {

class IInventoryPaneCallback;

class InventoryPane: public ScrollingPane 
{
	typedef ScrollingPane super;
public:
	InventoryPane(IInventoryPaneCallback* screen, Minecraft* mc, const IntRectangle& rect, int paneWidth, float clickMarginH, int numItems, int itemSize, int itemBorderSize);
	~InventoryPane();

	void tick();
	void renderBatch( std::vector<GridItem>& item, float alpha );
	bool onSelect( int gridId, bool selected );
	void drawScrollBar( ScrollBar& hScroll );

	void setRenderDecorations(bool value);

	IntRectangle rect;
	int paneWidth;
	IArea* _clickArea;
	IInventoryPaneCallback* screen;
	Minecraft* mc;

	int fillMarginX;
	int fillMarginY;

	int markerType;
	int markerIndex;
	float markerShare;
private:
	int lastItemIndex;
	int lastItemTicks;
    int BorderPixels;
	bool renderDecorations;

	IntRectangle bg;
};

class IInventoryPaneCallback
{
public:
	virtual ~IInventoryPaneCallback() {}
	virtual bool addItem(const InventoryPane* forPane, int index) = 0;
	virtual bool isAllowed( int slot ) = 0;
	virtual std::vector<const ItemInstance*> getItems(const InventoryPane* forPane) = 0;
};

}

#endif /*NET_MINECRAFT_CLIENT_GUI_COMPONENTS__InventoryPane_H__*/
