#include "ChestScreen.h"
#include "touch/TouchStartMenuScreen.h"
#include "../Screen.h"
#include "../components/NinePatch.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/ItemCategory.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/entity/item/ItemEntity.h"
#include "../../../world/level/Level.h"
#include "../../../locale/I18n.h"
#include "../../../util/StringUtils.h"
#include "../../../network/packet/ContainerSetSlotPacket.h"
#include "../../../network/RakNetInstance.h"
#include "../../../world/level/tile/entity/TileEntity.h"
#include "../../../world/level/tile/entity/ChestTileEntity.h"
#include "../../../world/inventory/ContainerMenu.h"
#include "../../../util/Mth.h"

//static NinePatchLayer* guiPaneFrame = NULL;

static __inline void setIfNotSet(bool& ref, bool condition) {
	ref = (ref || condition);
}

template<typename T,typename V>
T* upcast(V* x) { return x; }

static int heldMs = -1;
static int percent = -1;
static const float MaxHoldMs = 500.0f;
static const int MinChargeMs = 200;

class ItemDiffer {
public:
	ItemDiffer(int size)
	:	size(size),
		count(0)
	{
		base = new ItemInstance[size];
	}
	ItemDiffer(const std::vector<const ItemInstance*>& v)
	:	size(v.size()),
		count(0)
	{
		base = new ItemInstance[size];
		init(v);
	}

	~ItemDiffer() {
		delete[] base;
	}

	void init(const std::vector<const ItemInstance*>& v) {
		for (int i = 0; i < size; ++i) {
			if (v[i]) base[i] = *v[i];
			else      base[i].setNull();
		}
	}

	int getDiff(const std::vector<const ItemInstance*>& v, std::vector<int>& outIndices) {
		int diffLen = v.size() - size;
		int minLen = Mth::Max((int)v.size(), size);
		for (int i = 0; i < minLen; ++i) {
			//LOGI("%s, %s\n", base[i].toString().c_str(), v[i]?v[i]->toString().c_str() : "null");
			if (!ItemInstance::matchesNulls(&base[i], v[i]))
				outIndices.push_back(i);
		}
		return diffLen;
	}

private:
	int size;
	int count;
	ItemInstance* base;
};

const int   descFrameWidth = 100;

const int rgbActive = 0xfff0f0f0;
const int rgbInactive = 0xc0635558;
const int rgbInactiveShadow = 0xc0aaaaaa;

#ifdef __APPLE__
	static const float BorderPixels = 3;
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

typedef struct FlyingItem {
	ItemInstance item;
	float startTime;
	float sx, sy;
	float dx, dy;
} FlyingItem ;

static std::vector<FlyingItem> flyingItems;

ChestScreen::ChestScreen(Player* player, ChestTileEntity* chest)
:	super(new ContainerMenu(chest, chest->runningId)), //@huge @attn
	inventoryPane(NULL),
	chestPane(NULL),
	btnClose(4, ""),
	bHeader (5, "Inventory"),
	bHeaderChest (6, "Chest"),
	guiBackground(NULL),
	guiSlot(NULL),
	guiSlotMarked(NULL),
	guiSlotMarker(NULL),
	player(player),
	chest(chest),
	selectedSlot(-1),
	doRecreatePane(false)
	//guiSlotItem(NULL),
	//guiSlotItemSelected(NULL)
{
}

ChestScreen::~ChestScreen() {
	delete inventoryPane;
	delete chestPane;

	delete guiBackground;
	delete guiSlot;
	delete guiSlotMarked;
	delete guiSlotMarker;
	delete guiPaneFrame;

	delete menu;

	if (chest->clientSideOnly)
		delete chest;
}

void ChestScreen::init() {
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
	buttons.push_back(&bHeaderChest);
	buttons.push_back(&btnClose);

	// GUI - nine patches
	NinePatchFactory builder(minecraft->textures, "gui/spritesheet.png");

	guiBackground   = builder.createSymmetrical(IntRectangle(0, 0, 16, 16), 4, 4);
	guiSlot         = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 3, 3);
	guiSlotMarked   = builder.createSymmetrical(IntRectangle(0, 44, 8, 8), 3, 3);
	guiSlotMarker   = builder.createSymmetrical(IntRectangle(10, 42, 16, 16), 5, 5);
	guiPaneFrame    = builder.createSymmetrical(IntRectangle(28, 42, 4, 4), 1, 1)->exclude(4);
}

void ChestScreen::setupPositions() {
	// Left  - Categories
	bHeader.x = 0;
	bHeader.y = bHeaderChest.y = 0;
	bHeader.width = bHeaderChest.width = width / 2;// -  bDone.w;
	bHeaderChest.x = bHeader.x + bHeader.width;

	// Right  - Description
	btnClose.width = btnClose.height = 19;
	btnClose.x = width - btnClose.width;
	btnClose.y = 0;

	//guiPaneFrame->setSize((float)paneFuelRect.w + 2, (float)paneFuelRect.h + 4);
	guiBackground->setSize((float)width, (float)height);
	//guiSlotItem->setSize((float)width, 22); //@todo
	//guiSlotItemSelected->setSize((float)width, 22);

	setupPane();
}

void ChestScreen::tick() {
	if (inventoryPane)
		inventoryPane->tick();

	if (chestPane)
		chestPane->tick();

	if (doRecreatePane) {
		setupPane();
		doRecreatePane = false;
	}
}

void ChestScreen::handleRenderPane(Touch::InventoryPane* pane, Tesselator& t, int xm, int ym, float a) {
	if (pane) {
		int ms, id;
		pane->markerIndex = -1;
		if (pane->queryHoldTime(&id, &ms)) {
			heldMs = ms;

			FillingContainer* c = (pane == inventoryPane)?
					upcast<FillingContainer>(minecraft->player->inventory)
				:	upcast<FillingContainer>(chest);

			const int slotIndex = id + c->getNumLinkedSlots();
			ItemInstance* item = c->getItem(slotIndex);
			int count = (item && !item->isNull())? item->count : 0;
			float maxHoldMs = item? 700 + 10 * item->count: MaxHoldMs;

			if (count > 1) {
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

void ChestScreen::render(int xm, int ym, float a) {
	const int N = 5;
	static StopwatchNLast r(N);
	//renderBackground();

	Tesselator& t = Tesselator::instance;
	guiBackground->draw(t, 0, 0);
	glEnable2(GL_ALPHA_TEST);

	// Buttons (Left side + crafting)
	super::render(xm, ym, a);

	heldMs = -1;

	handleRenderPane(inventoryPane, t, xm, ym, a);
	handleRenderPane(chestPane, t, xm, ym, a);

	float now = getTimeS();
	float MaxTime = 0.3f;
	std::vector<FlyingItem> flyingToSave;

	glEnable2(GL_BLEND);
	glColor4f(1, 1, 1, 0.2f);
	t.beginOverride();
	//t.color(1.0f, 0.0f, 0.0f, 0.2f);
	//t.noColor();

	glEnable2(GL_SCISSOR_TEST);
	//LOGI("panesBox: %d, %d - %d, %d\n", panesBbox.x, panesBbox.y, panesBbox.w, panesBbox.h);
	minecraft->gui.setScissorRect(panesBbox);
	for (unsigned int i = 0; i < flyingItems.size(); ++i) {
		FlyingItem& fi = flyingItems[i];
		float since = (now - fi.startTime);
		if (since > MaxTime) continue;
		float t = since / MaxTime;
		t *= t;
		//float xx = fi.sx + t * 100.0f;

		float xx = Mth::lerp(fi.sx, fi.dx, t);
		float yy = Mth::lerp(fi.sy, fi.dy, t);
		ItemRenderer::renderGuiItem(minecraft->font, minecraft->textures, &fi.item, xx + 7, yy + 8, true);
		//minecraft->gui.renderSlotText(&fi.item, xx + 3, yy + 3, true, true);

		flyingToSave.push_back(fi);
	}
	t.enableColor();
	t.endOverrideAndDraw();
	glDisable2(GL_SCISSOR_TEST);

	flyingItems = flyingToSave;

	t.colorABGR(0xffffffff);
	glDisable2(GL_BLEND);

	minecraft->textures->loadAndBindTexture("gui/spritesheet.png");
}

void ChestScreen::buttonClicked(Button* button) {
	if (button == &btnClose) {
		minecraft->player->closeContainer();
	}
}

bool ChestScreen::handleAddItem(FillingContainer* from, FillingContainer* to, int itemIndex) {
	const int itemOffset = from->getNumLinkedSlots();
	const int slotIndex = itemIndex + itemOffset;
	ItemInstance* item = from->getItem(slotIndex);

	bool added = false;
	bool fromChest = (from == chest);
	Touch::InventoryPane* pane = fromChest? chestPane : inventoryPane;
	Touch::InventoryPane* toPane = fromChest? inventoryPane : chestPane;

	int wantedCount = (item && !item->isNull())? item->count * percent / 100 : 0;
	if ((item && !item->isNull()) && (!wantedCount || heldMs < MinChargeMs)) {
		wantedCount = 1;
	}

	if (wantedCount > 0) {
		ItemInstance takenItem(*item);
		takenItem.count = wantedCount;

		ItemDiffer differ(getItems(toPane));
		to->add(&takenItem);

		added = (takenItem.count != wantedCount);

		if (added) {
			item->count -= (wantedCount - takenItem.count);
			std::vector<int> changed;
			std::vector<const ItemInstance*> items = getItems(toPane);
			differ.getDiff(items, changed);

			ScrollingPane::GridItem g, gTo;
			pane->getGridItemFor_slow(itemIndex, g);

			//LOGI("Changed: %d\n", changed.size());
			for (unsigned int i = 0; i < changed.size(); ++i) {
				FlyingItem fi;
				fi.startTime = getTimeS();
				fi.item = *item;

				fi.sx = g.xf;
				fi.sy = g.yf;

				int toIndex = changed[i];
				toPane->getGridItemFor_slow(toIndex, gTo);

				fi.dx = gTo.xf;
				fi.dy = gTo.yf;
				flyingItems.push_back(fi);

				if (!fromChest && minecraft->level->isClientSide) {
					int j = toIndex;
					ItemInstance item = items[j]? *items[j] : ItemInstance();
					ContainerSetSlotPacket p(menu->containerId, j, item);
					minecraft->raknetInstance->send(p);
				}
			}
		}

		// Send to server, needs a bit special handling
		if (fromChest) {
			ItemInstance ins(item->count <= 0? ItemInstance() : *item);
			ContainerSetSlotPacket p(menu->containerId, slotIndex, ins);
			minecraft->raknetInstance->send(p);
		}
		if (item->count <= 0)
			from->clearSlot(slotIndex);
	}
	// Clear the marker indices
	pane->markerIndex = toPane->markerIndex = -1;

	return added;
}

bool ChestScreen::addItem(const Touch::InventoryPane* forPane, int itemIndex) {
	//LOGI("items.size, index: %d, %d\n", inventoryItems.size(), itemIndex);
	bool l2r = (forPane == inventoryPane);
	return handleAddItem(	l2r? upcast<FillingContainer>(minecraft->player->inventory) : upcast<FillingContainer>(chest),
							l2r? upcast<FillingContainer>(chest) : upcast<FillingContainer>(minecraft->player->inventory),
							itemIndex);
}

bool ChestScreen::isAllowed( int slot )
{
	return true;
}

bool ChestScreen::renderGameBehind()
{
	return false;
}

std::vector<const ItemInstance*> ChestScreen::getItems( const Touch::InventoryPane* forPane )
{
	if (forPane == inventoryPane) {
		for (int i = Inventory::MAX_SELECTION_SIZE, j = 0; i < minecraft->player->inventory->getContainerSize(); ++i, ++j)
			inventoryItems[j] = minecraft->player->inventory->getItem(i);
		return inventoryItems;
	}
	else {
		for (int i = 0; i < chest->getContainerSize(); ++i)
			chestItems[i] = chest->getItem(i);
		return chestItems;
	}
}


void ChestScreen::setupPane()
{
	inventoryItems.clear();
	for (int i = Inventory::MAX_SELECTION_SIZE; i < minecraft->player->inventory->getContainerSize(); ++i) {
		ItemInstance* item = minecraft->player->inventory->getItem(i);
		/*if (!item || item->isNull()) continue;*/
		inventoryItems.push_back(item);
	}
	chestItems.clear();
	for (int i = 0; i < chest->getContainerSize(); ++i) {
		ItemInstance* item = chest->getItem(i);
		/*if (!item || item->isNull()) continue;*/
		chestItems.push_back(item);
	}

	int maxWidth = width/2 - Bx/2;//- Bx - Bx/*- Bx*/;
	int InventoryColumns = maxWidth / ItemSize;
	const int realWidth = InventoryColumns * ItemSize;
	int paneWidth = realWidth;// + Bx + Bx;
	const int realBx = (width/2 - realWidth) / 2;
    
	IntRectangle rect(realBx,
#ifdef __APPLE__
		24 + By - ((width==240)?1:0), realWidth, ((width==240)?1:0) + height-By-By-24);
#else
		24 + By, realWidth, height-By-By-24);
#endif
	// IntRectangle(0, 0, 100, 100)
	if (inventoryPane) delete inventoryPane;
	inventoryPane = new Touch::InventoryPane(this, minecraft, rect, paneWidth, BorderPixels, minecraft->player->inventory->getContainerSize() - Inventory::MAX_SELECTION_SIZE, ItemSize, (int)BorderPixels);
	inventoryPane->fillMarginX = 0;
	inventoryPane->fillMarginY = 0;
	guiPaneFrame->setSize((float)rect.w + 2, (float)rect.h + 2);

	panesBbox = rect;
	rect.x += width/2;// - rect.w - Bx;
	panesBbox.w += (rect.x - panesBbox.x);

	if (chestPane) delete chestPane;
	chestPane = new Touch::InventoryPane(this, minecraft, rect, paneWidth, BorderPixels, chest->getContainerSize(), ItemSize, (int)BorderPixels);
	chestPane->fillMarginX = 0;
	chestPane->fillMarginY = 0;
	LOGI("Creating new panes\n:"
		"          Inventory %d %p\n"
		"          Chest     %d %p\n", (int)inventoryItems.size(), inventoryPane, (int)chestItems.size(), chestPane);
}

void ChestScreen::drawSlotItemAt( Tesselator& t, const ItemInstance* item, int x, int y, bool selected)
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
