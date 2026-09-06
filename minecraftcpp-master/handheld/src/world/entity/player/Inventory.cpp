#include "Inventory.h"
#include "../../level/material/Material.h"
#include "../../level/tile/QuartzBlockTile.h"
#include "../../level/tile/TreeTile.h"
#include "../../level/tile/StoneSlabTile.h"
#include "../../item/DyePowderItem.h"
#include "../../item/crafting/Recipe.h"
#include "../../item/CoalItem.h"
#include "../../level/tile/SandStoneTile.h"

Inventory::Inventory( Player* player, bool creativeMode )
:   super(	36 + Inventory::MAX_SELECTION_SIZE,
			MAX_SELECTION_SIZE,
			ContainerType::INVENTORY,
			creativeMode),
	player(player),
	selected(0)
{
	setupDefault();
	compressLinkedSlotList(0);
}

Inventory::~Inventory() {
}

ItemInstance* Inventory::getSelected() {
	return getLinked(selected);
}

void Inventory::selectSlot( int slot ) {
	if (slot < MAX_SELECTION_SIZE && slot >= 0)
		selected = slot;
}

bool Inventory::moveToSelectedSlot( int inventorySlot, bool propagate ) {
	return linkSlot(selected, inventorySlot, propagate);
}

int Inventory::getSelectionSize() {
	return MAX_SELECTION_SIZE;
}

void Inventory::setupDefault() {
	clearInventory();
	int Sel[MAX_SELECTION_SIZE] = {0};

#ifdef DEMO_MODE
	if (_isCreative) {
		Sel[0] = addItem(new ItemInstance(Item::shovel_stone));
		addItem(new ItemInstance(Item::pickAxe_stone));
		addItem(new ItemInstance(Item::hatchet_stone));
		addItem(new ItemInstance((Item*)Item::shears));
		addItem(new ItemInstance(Tile::ladder));
		Sel[3] = addItem(new ItemInstance(Tile::torch));
		addItem(new ItemInstance(Item::door_wood));

		Sel[4] = addItem(new ItemInstance(Tile::stoneBrick));
		Sel[5] = addItem(new ItemInstance(Tile::wood));
		Sel[2] = addItem(new ItemInstance(Tile::redBrick));
		Sel[1] = addItem(new ItemInstance(Tile::dirt));
		addItem(new ItemInstance(Tile::sandStone));
		addItem(new ItemInstance(Tile::gravel));
		addItem(new ItemInstance(Tile::rock));
		addItem(new ItemInstance(Tile::sand));
		//addItem(new ItemInstance(Tile::clay));

		addItem(new ItemInstance(Tile::cloth, 1, 15));
		addItem(new ItemInstance(Tile::cloth, 1, 14));
		addItem(new ItemInstance(Tile::cloth, 1, 13));
		Sel[7] = addItem(new ItemInstance(Tile::cloth, 1, 12));
		addItem(new ItemInstance(Tile::cloth, 1, 11));
		addItem(new ItemInstance(Tile::cloth, 1, 10));
		Sel[8] = addItem(new ItemInstance(Tile::cloth, 1, 9));
		addItem(new ItemInstance(Tile::cloth, 1, 8));
		Sel[6] = addItem(new ItemInstance(Tile::glass));
		addItem(new ItemInstance(Tile::thinGlass));
		addItem(new ItemInstance(Tile::stairs_stone));
        addItem(new ItemInstance(Tile::bookshelf));
        addItem(new ItemInstance(Tile::workBench));
		addItem(new ItemInstance(Tile::chest));
		addItem(new ItemInstance(Tile::furnace));

		addItem(new ItemInstance(((Tile*)Tile::flower)));
		addItem(new ItemInstance(Tile::cactus));

		//
		// Those below are inactive due to demo
		//
		addItem(new ItemInstance(Item::sword_stone));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 0));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 1));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 2));
		addItem(new ItemInstance(Tile::fence));
		addItem(new ItemInstance(Tile::fenceGate));
		addItem(new ItemInstance(Item::reeds));
		addItem(new ItemInstance(((Tile*)Tile::rose)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom2)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom1)));
		addItem(new ItemInstance(Tile::cloth, 1, 7));
		addItem(new ItemInstance(Tile::cloth, 1, 6));
		addItem(new ItemInstance(Tile::cloth, 1, 5));
		addItem(new ItemInstance(Tile::cloth, 1, 4));
		addItem(new ItemInstance(Tile::cloth, 1, 3));
		addItem(new ItemInstance(Tile::stairs_wood));
		addItem(new ItemInstance(Tile::goldBlock));
		addItem(new ItemInstance(Tile::ironBlock));
		addItem(new ItemInstance(Tile::emeraldBlock));
		addItem(new ItemInstance(Tile::lapisBlock));
		addItem(new ItemInstance(Tile::obsidian));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 0));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 1));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 2));
		addItem(new ItemInstance(Tile::stoneSlabHalf));
	} else {
#if defined(WIN32)
		// Survival
		addItem(new ItemInstance((Item*)Item::shears));
		addItem(new ItemInstance(Tile::redBrick));
		addItem(new ItemInstance(Tile::glass));
#endif
	}
#else
	if (_isCreative) {
		// Blocks
		Sel[1] = addItem(new ItemInstance(Tile::stoneBrick));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 0));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 1));
		addItem(new ItemInstance(Tile::stoneBrickSmooth, 1, 2));
		addItem(new ItemInstance(Tile::mossStone));
		Sel[5] =  addItem(new ItemInstance(Tile::wood));
		Sel[2] = addItem(new ItemInstance(Tile::redBrick));

#ifdef RPI
		Sel[3] = addItem(new ItemInstance(Tile::rock));
#else
		Sel[0] = addItem(new ItemInstance(Tile::rock));
#endif
		Sel[4] = addItem(new ItemInstance(Tile::dirt));
		addItem(new ItemInstance(Tile::grass));
		addItem(new ItemInstance(Tile::clay));
		addItem(new ItemInstance(Tile::sandStone, 1, 0));
		addItem(new ItemInstance(Tile::sandStone, 1, 1));
		addItem(new ItemInstance(Tile::sandStone, 1, 2));
		addItem(new ItemInstance(Tile::sand));
		addItem(new ItemInstance(Tile::gravel));
		
		Sel[7] = addItem(new ItemInstance(Tile::treeTrunk, 1, 0));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 1));
		addItem(new ItemInstance(Tile::treeTrunk, 1, 2));
		addItem(new ItemInstance(Tile::netherBrick));
		addItem(new ItemInstance(Tile::netherrack));
		addItem(new ItemInstance(Tile::stairs_stone));
		addItem(new ItemInstance(Tile::stairs_wood));
		Sel[6] = addItem(new ItemInstance(Tile::stairs_brick));
		addItem(new ItemInstance(Tile::stairs_sandStone));
		addItem(new ItemInstance(Tile::stairs_stoneBrickSmooth));
		addItem(new ItemInstance(Tile::stairs_netherBricks));
		addItem(new ItemInstance(Tile::stairs_quartz));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::STONE_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::COBBLESTONE_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::WOOD_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::BRICK_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SAND_SLAB));
		addItem(new ItemInstance(Tile::stoneSlabHalf, 1, StoneSlabTile::SMOOTHBRICK_SLAB));

		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_DEFAULT));
		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_LINES));
		addItem(new ItemInstance(Tile::quartzBlock, 1, QuartzBlockTile::TYPE_CHISELED));

		
		
		// Ores
		addItem(new ItemInstance(Tile::coalOre));
		addItem(new ItemInstance(Tile::ironOre));
		addItem(new ItemInstance(Tile::goldOre));
		addItem(new ItemInstance(Tile::emeraldOre));
		addItem(new ItemInstance(Tile::lapisOre));
		addItem(new ItemInstance(Tile::redStoneOre));

		addItem(new ItemInstance(Tile::goldBlock));
		addItem(new ItemInstance(Tile::ironBlock));
		addItem(new ItemInstance(Tile::emeraldBlock));
		addItem(new ItemInstance(Tile::lapisBlock));
		addItem(new ItemInstance(Tile::obsidian));
		addItem(new ItemInstance(Tile::snow));
		addItem(new ItemInstance(Tile::glass));
        addItem(new ItemInstance(Tile::lightGem));

		addItem(new ItemInstance(Tile::netherReactor));
		
		addItem(new ItemInstance(Tile::cloth, 1, 0));
		addItem(new ItemInstance(Tile::cloth, 1, 7));
		addItem(new ItemInstance(Tile::cloth, 1, 6));
		addItem(new ItemInstance(Tile::cloth, 1, 5));
		addItem(new ItemInstance(Tile::cloth, 1, 4));
		addItem(new ItemInstance(Tile::cloth, 1, 3));
		addItem(new ItemInstance(Tile::cloth, 1, 2));
		addItem(new ItemInstance(Tile::cloth, 1, 1));

		addItem(new ItemInstance(Tile::cloth, 1, 15));
		addItem(new ItemInstance(Tile::cloth, 1, 14));
		addItem(new ItemInstance(Tile::cloth, 1, 13));
		addItem(new ItemInstance(Tile::cloth, 1, 12));
		addItem(new ItemInstance(Tile::cloth, 1, 11));
		addItem(new ItemInstance(Tile::cloth, 1, 10));
		addItem(new ItemInstance(Tile::cloth, 1, 9));
		addItem(new ItemInstance(Tile::cloth, 1, 8));
		addItem(new ItemInstance(Tile::ladder));
#ifdef RPI
		addItem(new ItemInstance(Tile::torch));
#else
		Sel[3] = addItem(new ItemInstance(Tile::torch));
#endif
		addItem(new ItemInstance(Tile::thinGlass));

		addItem(new ItemInstance(Item::door_wood));
		addItem(new ItemInstance(Tile::trapdoor));
		addItem(new ItemInstance(Tile::fence));
		addItem(new ItemInstance(Tile::fenceGate));

		addItem(new ItemInstance(Item::bed));
		addItem(new ItemInstance(Tile::bookshelf));
		addItem(new ItemInstance(Item::painting));
		addItem(new ItemInstance(Tile::workBench));
		addItem(new ItemInstance(Tile::stonecutterBench));
		addItem(new ItemInstance(Tile::chest));
		addItem(new ItemInstance(Tile::furnace));
		addItem(new ItemInstance(Tile::tnt));

		addItem(new ItemInstance(((Tile*)Tile::flower)));
		addItem(new ItemInstance(((Tile*)Tile::rose)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom1)));
		addItem(new ItemInstance(((Tile*)Tile::mushroom2)));
		addItem(new ItemInstance(Tile::cactus));
		addItem(new ItemInstance(Tile::melon));
		addItem(new ItemInstance(Item::reeds));
		Sel[8] = addItem(new ItemInstance(Tile::sapling, 1, 0));
		addItem(new ItemInstance(Tile::sapling, 1, 1));
		addItem(new ItemInstance(Tile::sapling, 1, 2));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 0));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 1));
		addItem(new ItemInstance((Tile*)Tile::leaves, 1, 2));

		addItem(new ItemInstance(Item::seeds_wheat));
		addItem(new ItemInstance(Item::seeds_melon));
		addItem(new ItemInstance(Item::dye_powder, 1, DyePowderItem::WHITE));
		addItem(new ItemInstance(Item::hoe_iron));
#ifdef RPI
		Sel[0] = addItem(new ItemInstance(Item::sword_iron));
#else
		addItem(new ItemInstance(Item::sword_iron));
#endif
		addItem(new ItemInstance(Item::bow));
		addItem(new ItemInstance(Item::sign));
	} else {
#if defined(WIN32)
		// Survival
		addItem(new ItemInstance(Item::ironIngot, 64));
		addItem(new ItemInstance(Item::ironIngot, 34));
		addItem(new ItemInstance(Tile::stonecutterBench));
		addItem(new ItemInstance(Tile::workBench));
		addItem(new ItemInstance(Tile::furnace));
		addItem(new ItemInstance(Tile::wood, 54));
		addItem(new ItemInstance(Item::stick, 14));
		addItem(new ItemInstance(Item::coal, 31));
		addItem(new ItemInstance(Tile::sand, 6));
		addItem(new ItemInstance(Item::dye_powder, 23, DyePowderItem::PURPLE));
#endif
	}
#endif

	for (unsigned int i = 0; i < items.size(); ++i) {
		ItemInstance* item = items[i];

		if (i < MAX_SELECTION_SIZE) {
			if (item)
				LOGE("Error: Should not have items on slot %i\n", i);

			items[i] = NULL;
			continue;
		}

		if (item && _isCreative)
			item->count = 5;
	}

	for (int i = 0; i < MAX_SELECTION_SIZE; ++i) {
		linkedSlots[i] = LinkedSlot(Sel[i]);
	}

	//LOGI("Inventory has %d items\n", (int)items.size());
}

void Inventory::clearInventoryWithDefault()
{
	clearInventory();
	setupDefault();
}

int Inventory::getAttackDamage( Entity* entity )
{
	ItemInstance* item = getSelected();
	if (item != NULL) return item->getAttackDamage(entity);
	return 1;
}

bool Inventory::canDestroy( Tile* tile )
{
	if (tile->material->isAlwaysDestroyable()) return true;

	ItemInstance* item = getSelected();
	if (item != NULL) return item->canDestroySpecial(tile);
	return false;
}

float Inventory::getDestroySpeed( Tile* tile )
{
	ItemInstance* item = getSelected();
	if (item && item->id >= 256) {
		return Item::items[item->id]->getDestroySpeed(NULL, tile);
	}
	return 1.0f;
}

bool Inventory::moveToSelectionSlot( int selectionSlot, int inventorySlot, bool propagate ) {
	return linkSlot(selectionSlot, inventorySlot, propagate);
}

bool Inventory::moveToEmptySelectionSlot( int inventorySlot ) {
	return linkEmptySlot(inventorySlot);
}

void Inventory::doDrop( ItemInstance* item, bool randomly )
{
	player->drop(item, randomly);
}

bool Inventory::stillValid(Player* player) {
	if (this->player->removed) return false;
	if (player->distanceToSqr(this->player) > 8 * 8) return false;
	return true;
}

bool Inventory::add( ItemInstance* item ){
	if (_isCreative || player->hasFakeInventory)
		return true;

	return super::add(item);
}

bool Inventory::removeItem( const ItemInstance* samePtr ) {
	for (int i = MAX_SELECTION_SIZE; i < (int)items.size(); ++i) {
		if (items[i] == samePtr) {
			clearSlot(i);
			return true;
		}
	}
	return false;
}
