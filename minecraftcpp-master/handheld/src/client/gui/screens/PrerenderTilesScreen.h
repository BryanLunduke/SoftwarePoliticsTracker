#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__PrerenderTilesScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__PrerenderTilesScreen_H__

#include "../Screen.h"

#include "../../renderer/GameRenderer.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/level/tile/Tile.h"

#include "../../../world/entity/player/Inventory.h"
#include "../../renderer/Tesselator.h"
#include "../../../world/item/crafting/Recipes.h"
#include "../../../world/item/crafting/FurnaceRecipes.h"
#include "../../../world/level/tile/LeafTile.h"
#include "../../renderer/TileRenderer.h"

class PrerenderTilesScreen: public Screen
{
public:
	void init() {
		Player p(minecraft->level, true);
		Inventory _inventory(&p, true);
		Inventory* inventory = &_inventory;

		// Copy over the inventory items
		for (int i = Inventory::MAX_SELECTION_SIZE; i < inventory->getContainerSize(); ++i)
			addItem(inventory->getItem(i));

		// Fill the inventory with all the recipe items we don't already have: furnace
		const FurnaceRecipes::Map& furnaceRecipes = FurnaceRecipes::getInstance()->getRecipes();
		for (FurnaceRecipes::Map::const_iterator cit = furnaceRecipes.begin(); cit != furnaceRecipes.end(); ++cit) {
			ItemInstance ingredient(cit->first, 1, 0);
			addItem(&ingredient);
			ItemInstance result = cit->second;
			addItem(&result);
		}

		// Fill the inventory with all the recipe items we don't already have: crafting
		const RecipeList& recipes = Recipes::getInstance()->getRecipes();
		for (unsigned int i = 0; i < recipes.size(); ++i) {

			std::vector<ItemInstance> items;
			std::vector<ItemInstance> required = recipes[i]->getItemPack().getItemInstances();
			items.push_back(recipes[i]->getResultItem());
			items.insert(items.end(), required.begin(), required.end());

			for (unsigned int i = 0; i < items.size(); ++i) {
				ItemInstance& item = items[i];
				addItem(&item);
			}
		}

		// Manually added stuff
		// Example: the one that's spawned from tiles when destroyed
		int items[] = {
			Tile::sapling->id, LeafTile::BIRCH_LEAF,
			Tile::sapling->id, LeafTile::EVERGREEN_LEAF,
			Tile::sapling->id, LeafTile::NORMAL_LEAF,
			Tile::dirt->id, 0,
			Tile::reeds->id, 0,
			Tile::gravel->id, 0,
			Item::apple->id, 0,
			Tile::grass_carried->id, 0,
			Tile::web->id, 0,
			Item::sign->id, 0,
		};
		for (int i = 0; i < sizeof(items)/sizeof(int); i += 2) {
			ItemInstance item(items[i], 1, items[i+1]);
			addItem(&item);
		}
	}

	void render( int xm, int ym, float a ) {
		static Stopwatch w;
		w.start();

		glDisable2(GL_DEPTH_TEST);
		fill(0, 0, width, height, 0xffff00ff);
		//fill(0, 0, width, height, 0xff333333);
		glColor4f2(1, 1, 1, 1);
		glEnable2(GL_BLEND);

		LOGI("--------------------\n");
		/*int j = 0;
		for (int i = Inventory::MAX_SELECTION_SIZE; i < inventory->getContainerSize(); ++i) {

			ItemInstance* item = inventory->getItem(i);
			if (!item) continue;

			//LOGI("desc: %d - %s. %d\n", i, item->toString().c_str());

			int x = j%16 * 16;
			int y = j/16 * 16;

			//Tesselator::instance.color(0xffffffff);
			//minecraft->textures->loadAndBindTexture("gui/gui2.png");
			//glColor4f2(0.2f, 0.5f, 0.2f, 1);
			//blit(x, y, 4 + 20 * (i%9), 4, 16, 16, 15, 15);
			//glColor4f2(1, 1, 1, 1);

			if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
				LOGI("0, %d, %d, %d, 0\n", j, item->id, item->getAuxValue());
				ItemRenderer::renderGuiItemCorrect(minecraft->font, minecraft->textures, item, x, y);
			} else if (item->getIcon() >= 0) {
				LOGI("1, %d, %d, %d, %d\n", j, item->id, item->getAuxValue(), item->getIcon());
			}
			++j;
		}*/
		int j = 0;
		for(std::vector<ItemInstance>::iterator i = mItems.begin(); i != mItems.end(); ++i) {
			ItemInstance* item = &(*i);

			//LOGI("desc: %d - %s. %d\n", i, item->toString().c_str());

			int x = j%16 * 16;
			int y = j/16 * 16;
			if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
				LOGI("0, %d, %d, %d, 0\n", j, item->id, item->getAuxValue());
				ItemRenderer::renderGuiItemCorrect(minecraft->font, minecraft->textures, item, x, y);
			} else if (item->getIcon() >= 0) {
				LOGI("1, %d, %d, %d, %d\n", j, item->id, item->getAuxValue(), item->getIcon());
			}
			j++;
		}
		//@todo: blit out something famous here

		//glRotatef2(-180, 1, 0, 0);
		glEnable2(GL_DEPTH_TEST);
		glDisable2(GL_BLEND);

		w.stop();
		w.printEvery(100, "render-blocksel");
	}
	void removed(){}

	void addItem(ItemInstance* item) {
		if(item == NULL)
			return;
		if (item->getAuxValue() < 0) return;

		bool found = false;
		for(std::vector<ItemInstance>::iterator i = mItems.begin(); i != mItems.end(); ++i) {
			ItemInstance *jitem = &*i;
			if(jitem->id != item->id) continue;
			if(jitem->isStackedByData() && jitem->getAuxValue() != item->getAuxValue()) continue;

			found = true;
			break;
		}

		if (!found) {
			LOGI("Adding item: %s\n", item->getDescriptionId().c_str());
			mItems.push_back(*item);
		}
	}

private:
	std::vector<ItemInstance> mItems;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__PrerenderTilesScreen_H__*/
