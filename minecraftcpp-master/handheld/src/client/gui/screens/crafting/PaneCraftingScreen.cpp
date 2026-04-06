#include "PaneCraftingScreen.h"
#include "../touch/TouchStartMenuScreen.h"
#include "../../Screen.h"
#include "../../components/NinePatch.h"
#include "../../../Minecraft.h"
#include "../../../player/LocalPlayer.h"
#include "../../../renderer/Tesselator.h"
#include "../../../renderer/entity/ItemRenderer.h"
#include "../../../../world/item/Item.h"
#include "../../../../world/item/crafting/Recipes.h"
#include "../../../../world/item/ItemCategory.h"
#include "../../../../world/entity/player/Inventory.h"
#include "../../../../util/StringUtils.h"
#include "../../../../locale/I18n.h"
#include "../../../../world/entity/item/ItemEntity.h"
#include "../../../../world/level/Level.h"
#include "../../../../world/item/DyePowderItem.h"
#include "../../../../world/item/crafting/Recipe.h"

static NinePatchLayer* guiPaneFrame = NULL;

const float BorderPixels = 6.0f;
const int   descFrameWidth = 100;

const int rgbActive = 0xfff0f0f0;
const int rgbInactive = 0xc0635558;
const int rgbInactiveShadow = 0xc0aaaaaa;

class CategoryButton: public ImageButton {
	typedef ImageButton super;
public:
	CategoryButton(int id, const ImageButton* const* selectedPtr, NinePatchLayer* stateNormal, NinePatchLayer* statePressed)
	:	super(id, ""),
		selectedPtr(selectedPtr),
		stateNormal(stateNormal),
		statePressed(statePressed)
	{}

	void renderBg(Minecraft* minecraft, int xm, int ym) {
		//fill(x+1, y+1, x+w-1, y+h-1, 0xff999999);
		
		bool hovered = active && (minecraft->useTouchscreen()?
			(_currentlyDown && xm >= x && ym >= y && xm < x + width && ym < y + height) : false);

		if (hovered || *selectedPtr == this)
			statePressed->draw(Tesselator::instance, (float)x, (float)y);
		else
			stateNormal->draw(Tesselator::instance, (float)x, (float)y);
	}
	bool isSecondImage(bool hovered) { return false; }

private:
	const ImageButton* const* selectedPtr;
	NinePatchLayer* stateNormal;
	NinePatchLayer* statePressed;
};

PaneCraftingScreen::PaneCraftingScreen(int craftingSize)
:	craftingSize(craftingSize),
	currentCategory(-1),
	currentItem(NULL),
	pane(NULL),
	btnCraft(1),
	btnClose(2, ""),
	selectedCategoryButton(NULL),
	guiBackground(NULL),
	guiSlotCategory(NULL),
	guiSlotCategorySelected(NULL),
	numCategories(4)
{
	for (int i = 0; i < numCategories; ++i) {
		categoryBitmasks.push_back(1 << i);
		categoryIcons.push_back(i);
	}
}

PaneCraftingScreen::~PaneCraftingScreen() {
	for (unsigned int i = 0; i < _items.size(); ++i)
		delete _items[i];

	for (unsigned int i = 0; i < _categoryButtons.size(); ++i)
		delete _categoryButtons[i];

	clearCategoryItems();

	delete pane;
	delete guiBackground;

	// statics
	delete guiSlotCategory;
	delete guiSlotCategorySelected;
	delete guiPaneFrame;
}

void PaneCraftingScreen::init() {
	ImageDef def;
	def.name = "gui/spritesheet.png";
	def.x = 0;
	def.y = 1;
	def.width = def.height = 18;
	def.setSrc(IntRectangle(60, 0, 18, 18));
	btnClose.setImageDef(def, true);
	btnClose.scaleWhenPressed = false;

	btnCraft.init(minecraft->textures);

	buttons.push_back(&btnCraft);
	buttons.push_back(&btnClose);

	// GUI patches
	NinePatchFactory builder(minecraft->textures, "gui/spritesheet.png");

	guiBackground   = builder.createSymmetrical(IntRectangle(0, 0, 16, 16), 4, 4);
	guiPaneFrame    = builder.createSymmetrical(IntRectangle(0, 20, 8, 8), 1, 2)->setExcluded(1 << 4);
	guiSlotCategory = builder.createSymmetrical(IntRectangle(8, 32, 8, 8), 2, 2);
	guiSlotCategorySelected = builder.createSymmetrical(IntRectangle(0, 32, 8, 8), 2, 2);

	initCategories();
}

void PaneCraftingScreen::initCategories() {
	_categories.resize(numCategories);

	// Category buttons
	for (int i = 0; i < numCategories; ++i) {
		ImageButton* button = new CategoryButton(100 + i, &selectedCategoryButton, guiSlotCategory, guiSlotCategorySelected);
		_categoryButtons.push_back( button );
		buttons.push_back( button );
	}

	const RecipeList& all = Recipes::getInstance()->getRecipes();
	RecipeList filtered;
	filtered.reserve(all.size());

	// Apply size filter
	for (unsigned int i = 0; i < all.size(); ++i) {
		if (craftingSize >= all[i]->getCraftingSize())
			filtered.push_back(all[i]);
	}
	// Filter by subclass impl
	filterRecipes(filtered);

	// Add items from filtered recipes
	for (unsigned int i = 0; i < filtered.size(); ++i)
		addItem(filtered[i]);

	recheckRecipes();
}

void PaneCraftingScreen::setupPositions() {
	// Left  - Categories
	const int buttonHeight = (height - 16) / (Mth::Max(numCategories, 4));
	for (unsigned c = 0; c < _categoryButtons.size(); ++c) {
		ImageButton* button = _categoryButtons[c];
		button->x = (int)BorderPixels;
		button->y = (int)BorderPixels + c * (1 + buttonHeight);
		button->width = (int)buttonHeight;
		button->height = (int)buttonHeight;

		int icon = categoryIcons[c];
		ImageDef def;
		def.x = 0;
		def.width = def.height = (float)buttonHeight;
		def.name = "gui/spritesheet.png";
		def.setSrc(IntRectangle(32 * (icon/2), 64 + (icon&1) * 32, 32, 32));
		button->setImageDef(def, false);
	}
	// Right  - Description
	const int craftW = (int)(100 - 2 * BorderPixels - 0);
	btnCraft.x = width - descFrameWidth + (descFrameWidth-craftW)/2 - 1;//    width - descFrameWidth + (int)BorderPixels + 4;
	btnCraft.y = 20;
	btnCraft.setSize((float)craftW, 62);

	btnClose.width = btnClose.height = 19;
	btnClose.x = width - btnClose.width;
	btnClose.y = 0;

	// Middle - Scrolling pane
	paneRect.x = buttonHeight + 2 * (int)BorderPixels;
	paneRect.y = (int)BorderPixels + 2;
	paneRect.w = width - paneRect.x - descFrameWidth;
	paneRect.h = height - 2 * (int)BorderPixels - 4;

	guiPaneFrame->setSize((float)paneRect.w + 2, (float)paneRect.h + 4);
	guiBackground->setSize((float)width, (float)height);
	guiSlotCategory->setSize((float)buttonHeight, (float)buttonHeight);
	guiSlotCategorySelected->setSize((float)buttonHeight, (float)buttonHeight);

	int oldCategory = currentCategory;
	currentCategory = -1;
	buttonClicked(_categoryButtons[pane?oldCategory:0]);
}

void PaneCraftingScreen::tick() {
	if (pane) pane->tick();
}

void PaneCraftingScreen::render(int xm, int ym, float a) {
	const int N = 5;
	static StopwatchNLast r(N);
	//renderBackground();
	Tesselator& t = Tesselator::instance;
	guiBackground->draw(t, 0, 0);
	glEnable2(GL_ALPHA_TEST);

	// Buttons (Left side + crafting)
	super::render(xm, ym, a);

	// Mid
	r.start();
	// Blit frame
	guiPaneFrame->draw(t, (float)paneRect.x - 1, (float)paneRect.y - 2);
	if (pane) pane->render(xm, ym, a);
	r.stop();
	//r.printEvery(N, "test");

	const float slotWidth = (float)btnCraft.width / 2.0f;
	const float slotHeight = (float)btnCraft.height / 2.0f;
	const float slotBx = (float)btnCraft.x + slotWidth/2 - 8;
	const float slotBy = (float)btnCraft.y + slotHeight/2 - 9;

	ItemInstance reqItem;
	// Right side
	if (currentItem) {
		t.beginOverride();
		for (unsigned int i = 0; i < currentItem->neededItems.size(); ++i) {
			const float xx = slotBx + slotWidth  * (float)(i % 2);
			const float yy = slotBy + slotHeight * (float)(i / 2);
			CItem::ReqItem& req = currentItem->neededItems[i];
			reqItem = req.item;
			if (reqItem.getAuxValue() == -1) reqItem.setAuxValue(0);
			ItemRenderer::renderGuiItem(NULL, minecraft->textures, &reqItem, xx, yy, 16, 16, true);
		}
		t.endOverrideAndDraw();

		char buf[16];
		const float scale = 2.0f / 3.0f;
		const float invScale = 1.0f / scale;
		t.beginOverride();
		t.scale2d(scale, scale);
		for (unsigned int i = 0; i < currentItem->neededItems.size(); ++i) {
			const float xx =  4 + invScale * (slotBx + slotWidth  * (float)(i % 2));
			const float yy = 23 + invScale * (slotBy + slotHeight * (float)(i / 2));
			CItem::ReqItem& req = currentItem->neededItems[i];

			int bufIndex = 0;
			bufIndex += Gui::itemCountItoa(&buf[bufIndex], req.has);
			strcpy(&buf[bufIndex], "/"); bufIndex += 1;
			bufIndex += Gui::itemCountItoa(&buf[bufIndex], req.item.count);

			buf[bufIndex] = 0;
			if (req.enough())
				minecraft->font->drawShadow(buf, xx, yy, rgbActive);
			else {
				minecraft->font->draw(buf, xx+1, yy+1, rgbInactiveShadow);
				minecraft->font->draw(buf, xx, yy, rgbInactive);
			}
		}
		t.resetScale();
		t.endOverrideAndDraw();

		//minecraft->font->drawWordWrap(currentItemDesc, rightBx + 2, (float)btnCraft.y + btnCraft.h + 6, descFrameWidth-4, rgbActive);
		minecraft->font->drawWordWrap(currentItemDesc, (float)btnCraft.x, (float)(btnCraft.y + btnCraft.height + 6), (float)btnCraft.width, rgbActive);
	}
	//glDisable2(GL_ALPHA_TEST);
}

void PaneCraftingScreen::buttonClicked(Button* button) {
	if (button == &btnCraft)
		craftSelectedItem();

	if (button == &btnClose)
		minecraft->setScreen(NULL);

	// Did we click a category?
	if (button->id >= 100 && button->id < 200) {
		int categoryId = button->id - 100;
		ItemList& cat = _categories[categoryId];
		if (!cat.empty()) {
			onItemSelected(categoryId, cat[0]);
			pane->setSelected(0, true);
		}
		currentCategory = categoryId;
		selectedCategoryButton = (CategoryButton*)button;
	}
}

static void randomlyFillItemPack(ItemPack* ip, int numItems) {
	int added = 0;
	ItemInstance item(0, 1, 0);
	while (added < numItems) {
		int t = Mth::random(512);
		if (!Item::items[t]) continue;

		item.id = t;
		int id = ItemPack::getIdForItemInstance(&item);
		int count = Mth::random(10);
		for (int i = 0; i < count; ++i)
			ip->add(id);
		++added;
	}
}

static bool sortCanCraftPredicate(const CItem* a, const CItem* b) {
	//if (a->maxBuildCount == 0 && b->maxBuildCount > 0) return false;
	//if (b->maxBuildCount == 0 && a->maxBuildCount > 0) return true;
	return a->sortText < b->sortText;
}

void PaneCraftingScreen::recheckRecipes() {
	ItemPack ip;

	if (minecraft->player && minecraft->player->inventory) {
		Inventory* inv = (minecraft->player)->inventory;

		for (int i = Inventory::MAX_SELECTION_SIZE; i < inv->getContainerSize(); ++i) {
			if (ItemInstance* item = inv->getItem(i))
				ip.add(ItemPack::getIdForItemInstance(item), item->count);
		}
	} else {
		randomlyFillItemPack(&ip, 50);
	}

	ip.print();

	Stopwatch w;
	w.start();

	for (unsigned int i = 0; i < _items.size(); ++i) {
		CItem* item = _items[i];
		item->neededItems.clear();
		item->setCanCraft(true);

		Recipe* recipe = item->recipe;
		item->inventoryCount = ip.getCount(ItemPack::getIdForItemInstance(&item->item));
		//item->maxBuildCount = recipe->getMaxCraftCount(ip);
		// Override the canCraft thing, since I'm too lazy
		// to fix the above (commented out) function
		std::vector<ItemInstance> items = recipe->getItemPack().getItemInstances();
		for (unsigned int j = 0; j < items.size(); ++j) {
			ItemInstance& jtem = items[j];
			int has = 0;
			if (!Recipe::isAnyAuxValue(&jtem) && (jtem.getAuxValue() == Recipe::ANY_AUX_VALUE)) {
				// If the aux value on the item matters, but the recipe says it doesn't,
				// use this override (by fetching all items with aux-ids 0-15)
				ItemInstance aux(jtem);
				for (int i = 0; i < 16; ++i) {
					aux.setAuxValue(i);
					has += ip.getCount(ItemPack::getIdForItemInstance(&aux));
				}
			} else {
				// Else just use the normal aux-value rules
				has = ip.getCount(ItemPack::getIdForItemInstance(&jtem));
			}
			CItem::ReqItem req(jtem, has);
			item->neededItems.push_back(req);
			item->setCanCraft(item->canCraft() && req.enough());
		}
	}
	w.stop();
	w.printEvery(1, "> craft ");

	for (unsigned int c = 0; c < _categories.size(); ++c)
		std::stable_sort(_categories[c].begin(), _categories[c].end(), sortCanCraftPredicate);
}

void PaneCraftingScreen::addItem( Recipe* recipe )
{
	ItemInstance instance = recipe->getResultItem();
	Item* item = instance.getItem();
	CItem* ci = new CItem(instance, recipe, instance.getName());//item->getDescriptionId());
	if (item->id == Tile::cloth->id)
		ci->sortText = "Wool " + ci->text;
	if (item->id == Item::dye_powder->id)
		ci->sortText = "ZDye " + ci->text;
	_items.push_back(ci);

	if (item->category < 0)
		return;

	for (int i = 0; i < (int)categoryBitmasks.size(); ++i) {
		int bitmask = categoryBitmasks[i];
		if ((bitmask & item->category) != 0)
			_categories[i].push_back( ci );
	}
}

void PaneCraftingScreen::onItemSelected(const ItemPane* forPane, int itemIndexInCurrentCategory) {
	if (currentCategory >= (int)_categories.size()) return;
	if (itemIndexInCurrentCategory >= (int)_categories[currentCategory].size()) return;
	onItemSelected(currentCategory, _categories[currentCategory][itemIndexInCurrentCategory]);
}

void PaneCraftingScreen::onItemSelected(int buttonIndex, CItem* item) {
	currentItem = item;
	currentItemDesc = I18n::getDescriptionString(currentItem->item);

	if (buttonIndex != currentCategory) {
		// Clear item buttons for this category
		clearCategoryItems();

		// Setup new buttons for the items in this category
		const int NumCategoryItems = _categories[buttonIndex].size();

		if (pane) delete pane;
		pane = new ItemPane(this, minecraft->textures, paneRect, NumCategoryItems, height, minecraft->height);
		pane->f = minecraft->font;

		currentCategory = buttonIndex;
	}
}

void PaneCraftingScreen::clearCategoryItems()
{
	for (unsigned int i = 0; i < currentCategoryButtons.size(); ++i) {
		delete currentCategoryButtons[i];
	}
	currentCategoryButtons.clear();
}

void PaneCraftingScreen::keyPressed( int eventKey )
{
	if (eventKey == Keyboard::KEY_ESCAPE) {
		minecraft->setScreen(NULL);
		//minecraft->grabMouse();
	} else {
		super::keyPressed(eventKey);
	}
}

void PaneCraftingScreen::craftSelectedItem()
{
	if (!currentItem)
		return;
    if (!currentItem->canCraft())
        return;

	ItemInstance resultItem = currentItem->item;

	if (minecraft->player) {
		// Remove all items required for the recipe and ...
		for (unsigned int i = 0; i < currentItem->neededItems.size(); ++i) {
			CItem::ReqItem& req = currentItem->neededItems[i];

            // If the recipe allows any aux-value as ingredients, first deplete
            // aux == 0 from inventory. Since I'm not sure if this always is
            // correct, let's only do it for ingredient sandstone for now.
            ItemInstance toRemove = req.item;

            if (Tile::sandStone->id == req.item.id
             && Recipe::ANY_AUX_VALUE == req.item.getAuxValue()) {
                 toRemove.setAuxValue(0);
                 toRemove.count = minecraft->player->inventory->removeResource(toRemove, true);
                 toRemove.setAuxValue(Recipe::ANY_AUX_VALUE);
            }

            if (toRemove.count > 0) {
                minecraft->player->inventory->removeResource(toRemove);
            }
		}
		// ... add the new one! (in this order, to fill empty slots better)
		// if it doesn't fit, throw it on the ground!
		if (!minecraft->player->inventory->add(&resultItem)) {
			minecraft->player->drop(new ItemInstance(resultItem), false);
		}

		recheckRecipes();
	}
}

bool PaneCraftingScreen::renderGameBehind()
{
	return false;
}

bool PaneCraftingScreen::closeOnPlayerHurt() {
    return true;
}

void PaneCraftingScreen::filterRecipes(RecipeList& recipes) {
	for (int i = recipes.size() - 1; i >= 0; --i) {
		if (!filterRecipe(*recipes[i]))
			recipes.erase(recipes.begin() + i);
	}
}

const std::vector<CItem*>& PaneCraftingScreen::getItems(const ItemPane* forPane)
{
	return _categories[currentCategory];
}

void PaneCraftingScreen::setSingleCategoryAndIcon(int categoryBitmask, int categoryIcon) {
	assert(!minecraft && "setSingleCategoryAndIcon needs to be called from subclass constructor!\n");

	numCategories = 1;

	categoryIcons.clear();
	categoryIcons.push_back(categoryIcon);

	categoryBitmasks.clear();
	categoryBitmasks.push_back(categoryBitmask);
}

//
// Craft button
//
CraftButton::CraftButton( int id)
:	super(id, ""),
	bg(NULL),
	bgSelected(NULL),
	numItems(0)
{
}

CraftButton::~CraftButton()
{
	delete bg;
	delete bgSelected;
}

void CraftButton::setSize(float w, float h ) {
	this->width = (int)w;
	this->height = (int)h;

	if (bg && bgSelected) {
		bg->setSize(w, h);
		bgSelected->setSize(w, h);
	}
}

void CraftButton::init( Textures* textures)
{
	NinePatchFactory builder(textures, "gui/spritesheet.png");
	bg = builder.createSymmetrical(IntRectangle(112, 0, 8, 67), 2, 2);
	bgSelected = builder.createSymmetrical(IntRectangle(120, 0, 8, 67), 2, 2);
}

IntRectangle CraftButton::getItemPos( int i )
{
	return IntRectangle();
}

void CraftButton::renderBg(Minecraft* minecraft, int xm, int ym) {
	if (!bg || !bgSelected)
		return;
	//fill(x+1, y+1, x+w-1, y+h-1, 0xff999999);

	bool hovered = active && (minecraft->useTouchscreen()?
		(_currentlyDown && xm >= x && ym >= y && xm < x + width && ym < y + height) : false);

	if (hovered || selected)
		bgSelected->draw(Tesselator::instance, (float)x, (float)y);
	else
		bg->draw(Tesselator::instance, (float)x, (float)y);
}
