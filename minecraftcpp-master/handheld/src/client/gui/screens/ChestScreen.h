#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChestScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChestScreen_H__

#include "BaseContainerScreen.h"

#include "../components/InventoryPane.h"
#include "../components/Button.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;
class FillingContainer;
class ChestTileEntity;
class Tesselator;

class ChestScreen:	public BaseContainerScreen,
					public Touch::IInventoryPaneCallback
{
	typedef BaseContainerScreen super;
	typedef std::vector<CItem*> ItemList;
	friend class ItemPane;
public:
    ChestScreen(Player* player, ChestTileEntity* chest);
	~ChestScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
	void buttonClicked(Button* button);

	// IInventoryPaneCallback
	bool addItem(const Touch::InventoryPane* pane, int itemId);
	bool isAllowed( int slot );
	std::vector<const ItemInstance*> getItems( const Touch::InventoryPane* forPane );
	//const ItemList& getItems(const ItemPane* forPane);
private:
	void setupPane();

	void drawSlotItemAt(Tesselator& t, const ItemInstance* item, int x, int y, bool selected);
	bool handleAddItem(FillingContainer* from, FillingContainer* to, int itemIndex);
	void handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a);

	std::string currentItemDesc;
	ImageButton btnClose;
	Touch::THeader bHeader;
	Touch::THeader bHeaderChest;

	Touch::InventoryPane* inventoryPane;
	Touch::InventoryPane* chestPane;
	IntRectangle panesBbox;
	
	std::vector<const ItemInstance*> inventoryItems;
	std::vector<const ItemInstance*> chestItems;
	bool doRecreatePane;

	int selectedSlot;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlot;
	NinePatchLayer* guiSlotMarked;
	NinePatchLayer* guiSlotMarker;
	NinePatchLayer* guiPaneFrame;
	Player* player;
	ChestTileEntity* chest;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChestScreen_H__*/
