#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchIngameBlockSelectionScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchIngameBlockSelectionScreen_H__

#include "../../Screen.h"
#include "../../components/InventoryPane.h"
#include "../../components/Button.h"
#include "../../components/ScrollingPane.h"
#include "../../components/ItemPane.h"
#include "../../TweenData.h"
#include "../../../player/input/touchscreen/TouchAreaModel.h"
#include "../../../../AppPlatform.h"

namespace Touch {

class IngameBlockSelectionScreen :	public Screen,
									public IInventoryPaneCallback
{
	typedef Screen super;

public:
	IngameBlockSelectionScreen();
	virtual ~IngameBlockSelectionScreen();

	virtual void init();
	virtual void setupPositions();
	virtual void removed();

	void tick();
	void render(int xm, int ym, float a);

	bool hasClippingArea(IntRectangle& out);

	// IInventoryPaneCallback
	bool addItem(const InventoryPane* pane, int itemId);
	bool isAllowed(int slot);
	std::vector<const ItemInstance*> getItems(const InventoryPane* forPane);

	void buttonClicked(Button* button);
protected:
	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
private:
	void renderDemoOverlay();

	//int getLinearSlotId(int x, int y);
	int getSlotPosX(int slotX);
	int getSlotPosY(int slotY);

private:
	int selectedItem;
	bool  _pendingClose;
	InventoryPane* _blockList;

	THeader bHeader;
	ImageButton bDone;
	TButton bCraft;
	TButton bArmor;
	TButton bMenu;

	IntRectangle clippingArea;

	int InventoryRows;
	int InventorySize;
	int InventoryColumns;
};

}

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchIngameBlockSelectionScreen_H__*/
