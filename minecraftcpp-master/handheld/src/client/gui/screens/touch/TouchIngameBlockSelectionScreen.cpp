#include "TouchIngameBlockSelectionScreen.h"
#include "../crafting/WorkbenchScreen.h"
#include "../../Screen.h"
#include "../../components/ImageButton.h"
#include "../../components/InventoryPane.h"
#include "../../../gamemode/GameMode.h"
#include "../../../renderer/TileRenderer.h"
#include "../../../player/LocalPlayer.h"
#include "../../../renderer/gles.h"
#include "../../../renderer/entity/ItemRenderer.h"
#include "../../../renderer/Tesselator.h"
#include "../../../renderer/Textures.h"
#include "../../../Minecraft.h"
#include "../../../sound/SoundEngine.h"
#include "../../../../world/entity/player/Inventory.h"
#include "../../../../platform/input/Mouse.h"
#include "../../../../util/Mth.h"
#include "../../../../world/item/ItemInstance.h"
#include "../../../../world/entity/player/Player.h"
#include "../../../../world/item/crafting/Recipe.h"
#include "../../../player/input/touchscreen/TouchAreaModel.h"
#include "../ArmorScreen.h"

namespace Touch {

#if defined(__APPLE__)
    static const std::string demoVersionString("Not available in the Lite version");
#else
    static const std::string demoVersionString("Not available in the demo version");
#endif

#ifdef __APPLE__
    static const float BorderPixels = 4;
    #ifdef DEMO_MODE
        static const float BlockPixels = 22;
    #else
        static const float BlockPixels = 22;
    #endif
#else
    static const float BorderPixels = 4;
    static const float BlockPixels = 24;
#endif

static const int ItemSize = (int)(BlockPixels + 2*BorderPixels);

static const int Bx = 10; // Border Frame width
static const int By = 6; // Border Frame height
    
//
// Block selection screen
//
IngameBlockSelectionScreen::IngameBlockSelectionScreen() 
:	selectedItem(0),
	_blockList(NULL),
	_pendingClose(false),
	bArmor  (4, "Armor"),
	bDone   (3, ""),
	//bDone   (3, "Done"),
	bMenu   (2, "Menu"),
	bCraft  (1, "Craft"),
	bHeader (0, "Select blocks")
{
}

IngameBlockSelectionScreen::~IngameBlockSelectionScreen()
{
	delete _blockList;
}

void IngameBlockSelectionScreen::init()
{
	Inventory* inventory = minecraft->player->inventory;

	//const int itemWidth = 2 * BorderPixels + 

	int maxWidth = width - Bx - Bx;
	InventoryColumns = maxWidth / ItemSize;
	const int realWidth = InventoryColumns * ItemSize;
	const int realBx = (width - realWidth) / 2;

	IntRectangle rect(realBx,
#ifdef __APPLE__
		24 + By - ((width==240)?1:0), realWidth, ((width==240)?1:0) + height-By-By-20-24);
#else
		24 + By, realWidth, height-By-By-20-24);
#endif

	_blockList = new InventoryPane(this, minecraft, rect, width, BorderPixels, inventory->getContainerSize() - Inventory::MAX_SELECTION_SIZE, ItemSize, (int)BorderPixels);
	_blockList->fillMarginX = realBx;

	//for (int i = 0; i < inventory->getContainerSize(); ++i)
		//LOGI("> %d - %s\n", i, inventory->getItem(i)? inventory->getItem(i)->getDescriptionId().c_str() : "<-->\n");

	InventorySize = inventory->getContainerSize();
	InventoryRows = 1 + (InventorySize-1) / InventoryColumns;

    //
    // Buttons
    //
	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	bDone.setImageDef(def, true);
    bDone.width = bDone.height = 19;

	bDone.scaleWhenPressed = false;

	buttons.push_back(&bHeader);
	buttons.push_back(&bDone);
	if (!minecraft->isCreativeMode()) {
		buttons.push_back(&bCraft);
		buttons.push_back(&bArmor);
	}
}

void IngameBlockSelectionScreen::setupPositions() {
	bHeader.y = bDone.y = bCraft.y = 0;
	bDone.x   = width -  bDone.width;
	bCraft.x  = 0;//width - bDone.w - bCraft.w;
	bCraft.width = bArmor.width = 48;
	bArmor.x = bCraft.width;

	if (minecraft->isCreativeMode()) {
		bHeader.x = 0;
		bHeader.width = width;// -  bDone.w;
		bHeader.xText = width/2; // Center of the screen
	} else {
		bHeader.x = bCraft.width + bArmor.width;
		bHeader.width = width - bCraft.width - bArmor.width;// -  bDone.w;
		bHeader.xText = bHeader.x + (bHeader.width - bDone.width) /2;
	}

	clippingArea.x = 0;
	clippingArea.w = minecraft->width;
	clippingArea.y = 0;
	clippingArea.h = (int)(Gui::GuiScale * 24);
}

void IngameBlockSelectionScreen::removed()
{
	minecraft->gui.inventoryUpdated();
}

int IngameBlockSelectionScreen::getSlotPosX(int slotX) {
    // @todo: Number of columns
	return width / 2 - InventoryColumns * 10 + slotX * 20 + 2;
}

int IngameBlockSelectionScreen::getSlotPosY(int slotY) {
	return height - 16 - 3 - 22 * 2 - 22 * slotY;
}

void IngameBlockSelectionScreen::mouseClicked(int x, int y, int buttonNum) {
	_pendingClose = _blockList->_clickArea->isInside((float)x, (float)y);
	if (!_pendingClose)
		super::mouseClicked(x, y, buttonNum);
}

void IngameBlockSelectionScreen::mouseReleased(int x, int y, int buttonNum) {
	if (_pendingClose && _blockList->_clickArea->isInside((float)x, (float)y))
		minecraft->setScreen(NULL);
	else
		super::mouseReleased(x, y, buttonNum);
}

bool IngameBlockSelectionScreen::addItem(const InventoryPane* pane, int itemId)
{
	Inventory* inventory = minecraft->player->inventory;
	itemId += Inventory::MAX_SELECTION_SIZE;

	if (!inventory->getItem(itemId))
		return false;

	inventory->moveToSelectionSlot(0, itemId, true);

	inventory->selectSlot(0);
#ifdef __APPLE__
	minecraft->soundEngine->playUI("random.pop", 0.3f, 0.3f);//1.0f + 0.2f*(Mth::random()-Mth::random()));
#else
    minecraft->soundEngine->playUI("random.pop2", 1.0f, 0.3f);//1.0f + 0.2f*(Mth::random()-Mth::random()));
#endif

	// Flash the selected gui item
	minecraft->gui.flashSlot(inventory->selected);
    return true;
}

void IngameBlockSelectionScreen::tick()
{
	_blockList->tick();
	super::tick();
}

void IngameBlockSelectionScreen::render( int xm, int ym, float a )
{
	glDisable2(GL_DEPTH_TEST);
	glEnable2(GL_BLEND);

	Screen::render(xm, ym, a);
	_blockList->render(xm, ym, a);

	// render frame
	IntRectangle& bbox = _blockList->rect;
	Tesselator::instance.colorABGR(0xffffffff);
	minecraft->textures->loadAndBindTexture("gui/itemframe.png");
	glEnable2(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	blit(0, bbox.y-By, 0, 0, width, bbox.h+By+By, 215, 256); // why bbox.h + 1*B?
	glDisable2(GL_BLEND);

	glEnable2(GL_DEPTH_TEST);
}

void IngameBlockSelectionScreen::renderDemoOverlay() {
#ifdef DEMO_MODE
	fill(	getSlotPosX(0) - 3, getSlotPosY(1) - 3,
			getSlotPosX(9) - 3, getSlotPosY(-1) - 3, 0xa0 << 24);

	const int centerX = (getSlotPosX(4) + getSlotPosX(5)) / 2;
	const int centerY = (getSlotPosY(0) + getSlotPosY(1)) / 2 + 5;
	drawCenteredString(minecraft->font, demoVersionString, centerX, centerY, 0xffffffff);
#endif /*DEMO_MODE*/
}

void IngameBlockSelectionScreen::buttonClicked(Button* button) {
	if (button->id == bDone.id)
		minecraft->setScreen(NULL);

    if (button->id == bMenu.id)
        minecraft->screenChooser.setScreen(SCREEN_PAUSE);

	if (button->id == bCraft.id)
		minecraft->setScreen(new WorkbenchScreen(Recipe::SIZE_2X2));

	if (button == &bArmor)
		minecraft->setScreen(new ArmorScreen());
}

bool IngameBlockSelectionScreen::isAllowed( int slot )
{
	if (slot < 0 || slot >= minecraft->player->inventory->getContainerSize())
		return false;

#ifdef DEMO_MODE
	if (slot >= (minecraft->isCreativeMode()? 28 : 27)) return false;
#endif
	return true;
}

bool IngameBlockSelectionScreen::hasClippingArea( IntRectangle& out )
{
	out = clippingArea;
	return true;
}

std::vector<const ItemInstance*> IngameBlockSelectionScreen::getItems( const InventoryPane* forPane )
{
	std::vector<const ItemInstance*> out;
	for (int i = Inventory::MAX_SELECTION_SIZE; i < minecraft->player->inventory->getContainerSize(); ++i)
		out.push_back(minecraft->player->inventory->getItem(i));
	return out;
}

}
