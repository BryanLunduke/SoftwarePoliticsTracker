#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_PaneCraftingScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_PaneCraftingScreen_H__

#include "../../Screen.h"
#include "../../../../world/item/crafting/Recipes.h"
#include "../../../../world/item/ItemInstance.h"

#include "../../components/ScrollingPane.h"
#include "../../components/ImageButton.h"
#include "../../components/ItemPane.h"

class Font;
class CItem;
class Textures;
class NinePatchLayer;

class CraftButton:	public ImageButton
{
	typedef ImageButton super;
public:
	CraftButton(int id);
	~CraftButton();

	void init(Textures*);
	void setSize(float w, float h);

	void setNumItems(int i) { numItems = i; }
	IntRectangle getItemPos(int i);
	void renderBg(Minecraft* minecraft, int xm, int ym);
private:
	NinePatchLayer* bg;
	NinePatchLayer* bgSelected;
	int numItems;
};

class PaneCraftingScreen:	public Screen,
							public IItemPaneCallback
{
	typedef Screen super;
	typedef std::vector<CItem*> ItemList;
public:
    PaneCraftingScreen(int craftingSize);
	~PaneCraftingScreen();

    void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
	bool renderGameBehind();
    bool closeOnPlayerHurt();
	void buttonClicked(Button* button);
	void keyPressed( int eventKey );

	// IItemPaneCallback
	void onItemSelected(const ItemPane* forPane, int itemIndexInCurrentCategory);
	const std::vector<CItem*>& getItems(const ItemPane* forPane);
protected:
	void setSingleCategoryAndIcon(int categoryBitmask, int categoryIcon);
private:
	/// Filter out non craftable recipes.
	/// The default implementation calls bool filterRecipe(r) for every
	/// Recipe r and keeps the ones that returned true.
	/// A crafting size filter has already been applied.
	virtual void filterRecipes(RecipeList& recipes);
	virtual bool filterRecipe(const Recipe& recipe) = 0;

	void initCategories();

	void addItem(Recipe* recipe);
	void recheckRecipes();
	void onItemSelected(int buttonIndex, CItem* item);
	void clearCategoryItems();

	void craftSelectedItem();
	std::vector<ImageButton*> _categoryButtons;

	ItemList _items;
	std::vector<ItemList> _categories;

	int currentCategory;
	CItem* currentItem;
	std::string currentItemDesc;
	std::vector<Button*> currentCategoryButtons;
	ImageButton btnClose;
	CraftButton btnCraft;

	int craftingSize;

	ItemPane* pane;
	IntRectangle paneRect;
	//int paneX;
	//int paneW;

	int numCategories;
	std::vector<int> categoryBitmasks;
	std::vector<int> categoryIcons;
	ImageButton* selectedCategoryButton;

	// GUI elements such as 9-Patches
	NinePatchLayer* guiBackground;
	NinePatchLayer* guiSlotCategory;
	NinePatchLayer* guiSlotCategorySelected;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_CRAFT_PaneCraftingScreen_H__*/
