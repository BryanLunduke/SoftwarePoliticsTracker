#include "FurnaceScreen.h"
#include "crafting/PaneCraftingScreen.h"
#include "../Screen.h"
#include "../components/NinePatch.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/crafting/Recipes.h"
#include "../../../world/item/ItemCategory.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/entity/item/ItemEntity.h"
#include "../../../world/level/Level.h"
#include "../../../world/item/DyePowderItem.h"
#include "../../../world/item/crafting/FurnaceRecipes.h"
#include "../../../world/level/tile/entity/FurnaceTileEntity.h"
#include "../../../locale/I18n.h"
#include "../../../util/StringUtils.h"
#include "../../../world/inventory/FurnaceMenu.h"
#include "../../../network/packet/ContainerSetSlotPacket.h"
#include "../../../network/RakNetInstance.h"

static int heldMs = -1;
static int percent = -1;
static const float MaxHoldMs = 500.0f;
static const int MinChargeMs = 200;

static void setIfNotSet(bool& ref, bool condition) {
	ref = (ref || condition);
}

const int   descFrameWidth = 100;

const int rgbActive = 0xfff0f0f0;
const int rgbInactive = 0xc0635558;
const int rgbInactiveShadow = 0xc0aaaaaa;

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


FurnaceScreen::FurnaceScreen(Player* player, FurnaceTileEntity* furnace)
:	super(new FurnaceMenu(furnace)), //@huge @attn
	inventoryPane(NULL),
	btnFuel(FurnaceTileEntity::SLOT_FUEL),
	btnIngredient(FurnaceTileEntity::SLOT_INGREDIENT),
	btnResult(FurnaceTileEntity::SLOT_RESULT),
	btnClose(4, ""),
	bHeader (5, "Furnace"),
	guiBackground(NULL),
	guiSlot(NULL),
	guiSlotMarked(NULL),
	guiSlotMarker(NULL),
	guiPaneFrame(NULL),
	player(player),
	furnace(furnace),
	selectedSlot(FurnaceTileEntity::SLOT_INGREDIENT),
	doRecreatePane(false),
	lastBurnTypeId(0),
	descWidth(90)
	//guiSlotItem(NULL),
	//guiSlotItemSelected(NULL)
{
	//LOGI("Creating FurnaceScreen with %p, %d\n", furnace, furnace->runningId);
}

FurnaceScreen::~FurnaceScreen() {
	clearItems();

	delete inventoryPane;

	delete guiBackground;
	delete guiSlot;
	delete guiSlotMarked;
	delete guiSlotMarker;
	delete guiPaneFrame;

	delete menu;

	if (furnace->clientSideOnly)
		delete furnace;
}

void FurnaceScreen::init() {
	super::init();
	//printf("-> %d\n", width/2);

	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	btnClose.setImageDef(def, true);
	btnClose.scaleWhenPressed = false;

	buttons.push_back(&bHeader);
	buttons.push_back(&btnIngredient);
	buttons.push_back(&btnFuel);
	buttons.push_back(&btnResult);
	buttons.push_back(&btnClose);

	// GUI - nine patches
	NinePatchFactory builder(minecraft->textures, "gui/spritesheet.png");

	guiBackground   = builder.createSymmetrical(IntRectangle(0, 0, 16, 16), 4, 4);
	guiSlot         = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 3, 3);
	guiSlotMarked   = builder.createSymmetrical(IntRectangle(0, 44, 8, 8), 3, 3);
	guiSlotMarker   = builder.createSymmetrical(IntRectangle(10, 42, 16, 16), 5, 5);
	guiPaneFrame    = builder.createSymmetrical(IntRectangle(28, 42, 4, 4), 1, 1)->setExcluded(1 << 4);

	recheckRecipes();
}

void FurnaceScreen::setupPositions() {
	// Left  - Categories
	bHeader.x = bHeader.y = 0;
	bHeader.width = width;// -  bDone.w;

	btnClose.width = btnClose.height = 19;
	btnClose.x = width - btnClose.width;
	btnClose.y = 0;

	// Inventory pane
	const int maxWidth = width/2 - Bx - Bx;
	const int InventoryColumns = maxWidth / ItemSize;
	const int realWidth = InventoryColumns * ItemSize;
	const int paneWidth = realWidth + Bx + Bx;
	const int realBx = (paneWidth - realWidth) / 2;

	inventoryPaneRect = IntRectangle(realBx,
#ifdef __APPLE__
		26 + By - ((width==240)?1:0), realWidth, ((width==240)?1:0) + height-By-By-28);
#else
		26 + By, realWidth, height-By-By-28);
#endif

	// Right - Slots, description etc
	{
	int cx = (inventoryPaneRect.x + inventoryPaneRect.w);
	int rightWidth = width - cx;

	btnIngredient.width = btnFuel.width = btnResult.width = (int)guiSlot->getWidth();
	btnIngredient.height = btnFuel.height = btnResult.height = (int)guiSlot->getHeight();
	int space = rightWidth - (2 * btnFuel.width + 40);
	int margin = space/2;
	int bx0 = cx + margin;
	int bx1 = width - margin - btnFuel.width;
	btnIngredient.x = btnFuel.x = bx0;
	descWidth = (float)Mth::Min(90, width - bx1 + 24 - 4);

	int by = 36;// + (height-20) / 6;
	btnIngredient.y = by;
	btnFuel.y = by + 20 + btnFuel.height;

	btnResult.x = bx1;
	btnResult.y = (btnIngredient.y + btnFuel.y) / 2;

	guiBackground->setSize((float)width, (float)height);
	guiSlotMarker->setSize((float)btnFuel.width + 4, (float)btnFuel.height + 4);

	recheckRecipes();
	setupInventoryPane();
	}
}

void FurnaceScreen::tick() {
	if (inventoryPane)
		inventoryPane->tick();

	if (doRecreatePane) {
		recheckRecipes();
		setupInventoryPane();
		doRecreatePane = false;
	}
}

void FurnaceScreen::handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a) {
	if (pane) {
		int ms, id;
		pane->markerIndex = -1;
		if (pane->queryHoldTime(&id, &ms)) {
			heldMs = ms;
			
			const ItemInstance* item = inventoryItems[id];
			int count = (item && !item->isNull())? item->count : 0;
			float maxHoldMs = item? 700 + 10 * item->count: MaxHoldMs;

			if (count > 1 && canMoveToFurnace(id, item)) {
				float share = (heldMs-MinChargeMs) / maxHoldMs;
				pane->markerType = 1;//(heldMs >= MinChargeMs)? 1 : 0;
				pane->markerIndex = id;
				pane->markerShare = Mth::Max(share, 0.0f);

				percent = (int)Mth::clamp(100.0f * share, 0.0f, 100.0f);
				if (percent >= 100) {
					addItem(pane, id);
				}
			}
		}

		pane->render(xm, ym, a);
		guiPaneFrame->draw(t, (float)(pane->rect.x - 1), (float)(pane->rect.y - 1));
		//LOGI("query-iv: %d, %d\n", gridId, heldMs);

	}
}

void FurnaceScreen::render(int xm, int ym, float a) {
	const int N = 5;
	static StopwatchNLast r(N);
	//renderBackground();

	updateResult(furnace->getItem(FurnaceTileEntity::SLOT_INGREDIENT));

	Tesselator& t = Tesselator::instance;
	guiBackground->draw(t, 0, 0);
	glEnable2(GL_ALPHA_TEST);

	// Buttons (Left side + crafting)
	super::render(xm, ym, a);

	handleRenderPane(inventoryPane, t, xm, ym, a);

	t.colorABGR(0xffffffff);

	drawSlotItemAt(t, furnace->getItem(btnIngredient.id), btnIngredient.x, btnIngredient.y, btnIngredient.id == selectedSlot);
	drawSlotItemAt(t, furnace->getItem(btnFuel.id), btnFuel.x, btnFuel.y, btnFuel.id == selectedSlot);

	const ItemInstance* resultSlotItem = furnace->getItem(btnResult.id);
	drawSlotItemAt(t, resultSlotItem, btnResult.x, btnResult.y, btnResult.id == selectedSlot);

	if (!burnResult.isNull()) {
		if (!resultSlotItem || resultSlotItem->isNull()) {
			glEnable2(GL_BLEND);
			t.beginOverride();
			t.colorABGR(0x33ffffff);
			t.noColor();
			ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &burnResult, (float)(btnResult.x + 7), (float)(btnResult.y + 8), true);
			t.endOverrideAndDraw();
			glDisable2(GL_BLEND);
		}
		minecraft->font->drawWordWrap(currentItemDesc, (float)btnResult.x - 24, (float)(btnResult.y + btnResult.height + 6), descWidth, rgbActive);
	}

	minecraft->textures->loadAndBindTexture("gui/spritesheet.png");
	int yy = btnResult.y + 8;
	int fpx = furnace->getLitProgress(14) + 2;
	int xx0 = btnIngredient.x + 8;
	blit(xx0, yy, 80, 40, 16, 16, 32, 32);
	blit(xx0, yy + 16 - fpx, 112, 40 + 32-fpx-fpx, 16, fpx, 32, fpx+fpx);

	int bpx = furnace->getBurnProgress(24);
	int xx1 = btnIngredient.x + 40;
	blit(xx1, yy, 144, 40, 24, 16, 48, 32);
	blit(xx1, yy, 144, 72, bpx, 16, bpx+bpx, 32);
}

void FurnaceScreen::buttonClicked(Button* button) {
	int slot = button->id;

	if (button == &btnClose) {
		minecraft->player->closeContainer();
	}

	if (slot >= FurnaceTileEntity::SLOT_INGREDIENT
	 && slot <= FurnaceTileEntity::SLOT_RESULT) {
		// Can't highlight the Result slot
		int oldSlot = selectedSlot;
		if (slot != FurnaceTileEntity::SLOT_RESULT)
			selectedSlot = slot;

		if (oldSlot == selectedSlot)
			takeAndClearSlot(slot);
	}
}

static bool sortCanCraftPredicate(const CItem* a, const CItem* b) {
	//if (a->maxBuildCount == 0 && b->maxBuildCount > 0) return false;
	//if (b->maxBuildCount == 0 && a->maxBuildCount > 0) return true;
	return a->sortText < b->sortText;
}

void FurnaceScreen::recheckRecipes()
{
	clearItems();

	Stopwatch w;
	w.start();

	const FurnaceRecipes* recipes = FurnaceRecipes::getInstance();
	ItemPack ip;
	// Check for fuel, and items to burn
	if (minecraft->player && minecraft->player->inventory) {
		Inventory* inv = (minecraft->player)->inventory;

		for (int i = Inventory::MAX_SELECTION_SIZE; i < inv->getContainerSize(); ++i) {
			if (ItemInstance* item = inv->getItem(i)) {
				// Fuel material
				if (FurnaceTileEntity::isFuel(*item)) {
					CItem* ci = new CItem(*item, NULL, "");//item->getName());
					//LOGI("Adding fuel: %s\n", item->getName());
					listFuel.push_back(ci);
				}
				// Ingredient/burn material
				if (recipes->isFurnaceItem(item->id)) {
					CItem* ci = new CItem(*item, NULL, "");//item->getName());
					//LOGI("Adding item to burn: %s\n", item->getName());
					listIngredient.push_back(ci);
				}
				//ip.add(ItemPack::getIdForItemInstance(item), item->count);
			}
		}
	}

	ip.print();

	w.stop();
	w.printEvery(1, "> craft ");

	updateItems();
	//std::stable_sort(_categories[c].begin(), _categories[c].end(), sortCanCraftPredicate);
}

bool FurnaceScreen::addItem(const Touch::InventoryPane* forPane, int itemIndex) {
	//LOGI("items.size, index: %d, %d\n", inventoryItems.size(), itemIndex);
	const ItemInstance* item = inventoryItems[itemIndex];
	if (!item || item->isNull()) return false;

	setIfNotSet(doRecreatePane, handleAddItem(selectedSlot, item));

	if (doRecreatePane) {
		int slot = inventorySlots[itemIndex];
		if (player->inventory->getItem(slot)) {
			// fix: if we deplete a slot we didn't click on (but had same type),
			// we need to NULLify the slot that actually was depleted
			for (unsigned int i = 0; i < inventorySlots.size(); ++i) {
				slot = inventorySlots[i];
				if (!player->inventory->getItem(slot)) {
					LOGI("Changed! removing slot %d (was: %d)\n", i, itemIndex);
					itemIndex = i;
					break;
				}
			}
		}
		inventoryItems[itemIndex] = NULL;
	}
	//LOGI("Pressed button: %d\n", itemIndexInCurrentCategory);
	return true;
}

bool FurnaceScreen::isAllowed( int slot )
{	//LOGI("items.size, index: %d, %d\n", inventoryItems.size(), slot);
	if (slot >= (int)inventoryItems.size()) return false;
    if (!inventoryItems[slot]) return false;
	const ItemInstance& item = *inventoryItems[slot];

	if (selectedSlot == btnFuel.id)
		return (FurnaceTileEntity::getBurnDuration(item) > 0);
	else
    if (selectedSlot == btnIngredient.id)
		return !FurnaceRecipes::getInstance()->getResult(item.id).isNull();
	return false;
}

bool FurnaceScreen::renderGameBehind()
{
	return false;
}

std::vector<const ItemInstance*> FurnaceScreen::getItems( const Touch::InventoryPane* forPane )
{
	return inventoryItems;
}

void FurnaceScreen::clearItems()
{
	for (unsigned int i = 0; i < listFuel.size(); ++i) delete listFuel[i];
	for (unsigned int i = 0; i < listIngredient.size(); ++i) delete listIngredient[i];
	listFuel.clear();
	listIngredient.clear();
}

void FurnaceScreen::updateItems() {
	inventoryItems.clear();
	inventorySlots.clear();

	ItemList all(listFuel.begin(), listFuel.end());
	all.insert(all.end(), listIngredient.begin(), listIngredient.end());
	for (int i = Inventory::MAX_SELECTION_SIZE; i < minecraft->player->inventory->getContainerSize(); ++i) {
		ItemInstance* item = minecraft->player->inventory->getItem(i);
		if (!item) continue;
		//LOGI("ItemInstance (%p) Id/aux/count: %d, %d, %d\n", item, item->id, item->getAuxValue(), item->count);
		bool added = false;
		for (unsigned int j = 0; j < listFuel.size(); ++j) {
			if (ItemInstance::matches(item, &listFuel[j]->item)) {
				inventorySlots.push_back(i);
				inventoryItems.push_back(item);
				added = true;
				break;
			}
		}
		if (added) continue;
		for (unsigned int j = 0; j < listIngredient.size(); ++j) {
			if (ItemInstance::matches(item, &listIngredient[j]->item)) {
				inventorySlots.push_back(i);
				inventoryItems.push_back(item);
				added = true;
				break;
			}
		}
	}
}

bool FurnaceScreen::canMoveToFurnace(int inventorySlot, const ItemInstance* item) {
	if (!isAllowed(inventorySlot)) return false;
	ItemInstance* jitem = furnace->getItem(selectedSlot);
	if (!jitem || jitem->isNull()) return true;
	if (ItemInstance::isStackable(item, jitem) && jitem->count < jitem->getMaxStackSize())
		return true;

	return false;
}

void FurnaceScreen::updateResult( const ItemInstance* item )
{
	const ItemInstance* result = furnace->getItem(FurnaceTileEntity::SLOT_RESULT);
	if (!result->isNull()) {
		int id = result->id;
		if (id == lastBurnTypeId) return;
		currentItemDesc = I18n::getDescriptionString(*result);
		lastBurnTypeId = id;
		this->burnResult = *result;
	} else {
		int id = (item? item->id : 0);
		if (id == lastBurnTypeId) return;

		ItemInstance burnResult = FurnaceRecipes::getInstance()->getResult(id);
		if (!burnResult.isNull())
			currentItemDesc = I18n::getDescriptionString(burnResult);
		else
			currentItemDesc = "";
		lastBurnTypeId = id;
		this->burnResult = burnResult;
	}
}

void FurnaceScreen::setupInventoryPane()
{
	// IntRectangle(0, 0, 100, 100)
	if (inventoryPane) delete inventoryPane;
	inventoryPane = new Touch::InventoryPane(this, minecraft, inventoryPaneRect, inventoryPaneRect.w, BorderPixels, inventoryItems.size(), ItemSize, (int)BorderPixels);
	inventoryPane->fillMarginX = 0;
	inventoryPane->fillMarginY = 0;
	guiPaneFrame->setSize((float)inventoryPaneRect.w + 2, (float)inventoryPaneRect.h + 2);
	//LOGI("Creating new pane: %d %p\n", inventoryItems.size(), inventoryPane);
}

void FurnaceScreen::drawSlotItemAt( Tesselator& t, const ItemInstance* item, int x, int y, bool selected)
{
	float xx = (float)x;
	float yy = (float)y;

	(selected? guiSlot/*Marked*/ : guiSlot)->draw(t, xx, yy);

	if (selected)
		guiSlotMarker->draw(t, xx - 2, yy - 2);

	if (item && !item->isNull()) {
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, item, xx + 7, yy + 8, true);
		minecraft->gui.renderSlotText(item, xx + 3, yy + 3, true, true);
	}
}

ItemInstance FurnaceScreen::moveOver(const ItemInstance* item, int maxCount) {
	int wantedCount = item->count * percent / 100;
	if (!wantedCount || heldMs < MinChargeMs)
		wantedCount = 1;

	wantedCount = Mth::Min(wantedCount, maxCount);

	ItemInstance removed(item->id, wantedCount, item->getAuxValue());
	int oldSize = minecraft->player->inventory->getNumEmptySlots();
	if (minecraft->player->inventory->removeResource(removed)) {
		int newSize = minecraft->player->inventory->getNumEmptySlots();
		setIfNotSet(doRecreatePane, newSize != oldSize);
		return removed;
	}
	return ItemInstance();
}

void FurnaceScreen::takeAndClearSlot( int slot )
{
	//if (selectedSlot == btnBurn.id && !furnace->isSlotEmpty(btnBurn.id))
	ItemInstance oldItem = *furnace->getItem(slot);
	ItemInstance blank;

	furnace->setItem(slot, &blank);
	if (minecraft->level->isClientSide) {
		ContainerSetSlotPacket p(menu->containerId, slot, blank);
		minecraft->raknetInstance->send(p);
	}

	int oldSize = minecraft->player->inventory->getNumEmptySlots();

	if (!minecraft->player->inventory->add(&oldItem))
		minecraft->player->drop(new ItemInstance(oldItem), false);

	int newSize = minecraft->player->inventory->getNumEmptySlots();
	setIfNotSet(doRecreatePane, newSize != oldSize);
}

bool FurnaceScreen::handleAddItem( int slot, const ItemInstance* item )
{
	ItemInstance* furnaceItem = furnace->getItem(slot);
	int oldSize = minecraft->player->inventory->getNumEmptySlots();

	if (item->id == furnaceItem->id) {
		// If stackable, stack them! Else deny the addition
		const int maxMovedCount = furnaceItem->getMaxStackSize() - furnaceItem->count;
		if (maxMovedCount <= 0)
			return false;

		ItemInstance added = moveOver(item, maxMovedCount);
		furnaceItem->count += added.count;
	} else {
		if (!furnace->isSlotEmpty(slot))
			return false;//takeAndClearSlot(slot);

		ItemInstance moved = moveOver(item, item->getMaxStackSize());
		player->containerMenu->setSlot(slot, &moved);
	}
	 
	if (minecraft->level->isClientSide) {
		ContainerSetSlotPacket p(menu->containerId, slot, *furnaceItem);
		minecraft->raknetInstance->send(p);
	}

	int newSize = minecraft->player->inventory->getNumEmptySlots();
	return (newSize != oldSize);
}
