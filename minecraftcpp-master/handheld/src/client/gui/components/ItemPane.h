#ifndef ITEMPANE_H__
#define ITEMPANE_H__

#include <string>
#include <vector>
#include "ScrollingPane.h"
#include "../../../world/item/ItemInstance.h"

class Font;
class Textures;
class NinePatchLayer;
class Recipe;
class ItemPane;

class CItem
{
public:
	CItem(const ItemInstance& ins, Recipe* recipe, const std::string& text)
	:	item(ins),
		recipe(recipe),
		text(text),
		sortText(text),
		//maxBuildCount(0),
		numBuilt(0),
		inventoryCount(0),
		_canCraft(false)
	{
	}

	typedef struct ReqItem {
		ReqItem() {}
		ReqItem(const ItemInstance& needItem, int has)
			:	item(needItem), has(has) {}
		ItemInstance item;
		int has;
		bool enough() { return has >= item.count; }
	} ReqItem;

    bool canCraft() {
        return _canCraft;// || maxBuildCount > 0;
    }
	void setCanCraft(bool status) {
		_canCraft = status;
	}
    
	ItemInstance item;
	Recipe*		recipe;
	std::string text;
	std::string sortText;
	//int			maxBuildCount;
	int			numBuilt;
	int			inventoryCount;
	std::vector<ReqItem> neededItems;
private:
	bool		_canCraft;
};

class IItemPaneCallback
{
public:
	virtual ~IItemPaneCallback() {}
	virtual void onItemSelected(const ItemPane* forPane, int index) = 0;
	virtual const std::vector<CItem*>& getItems(const ItemPane* forPane) = 0;
};

class ItemPane: public ScrollingPane
{
	typedef ScrollingPane super;
public:
	ItemPane(	IItemPaneCallback* screen,
				Textures* textures,
				const IntRectangle& rect,
				int numItems,
				int guiHeight,
				int physicalScreenHeight,
				bool isVertical = true);
	~ItemPane();

	void renderBatch( std::vector<GridItem>& item, float alpha );
	bool onSelect( int gridId, bool selected );
	void drawScrollBar( ScrollBar& hScroll );
	//void setSize()

	Font* f;
	Textures* textures;
	IItemPaneCallback* screen;

	int physicalScreenHeight; // Needed for glScissor
	bool isVertical;

	NinePatchLayer* guiSlotItem;
	NinePatchLayer* guiSlotItemSelected;
};

#endif /*ITEMPANE_H__*/
