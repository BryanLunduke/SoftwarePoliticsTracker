#include "CraftingFilters.h"
#include "../../../../world/item/ItemInstance.h"
#include "../../../../world/item/Item.h"
#include "../../../../world/level/tile/Tile.h"
#include "../../../../world/level/material/Material.h"
#include "../../../../world/level/tile/StoneSlabTile.h"

namespace CraftingFilters {

bool isStonecutterItem(const ItemInstance& ins) {
	Item* const item = ins.getItem();
	if (item->id < 0 || item->id >= 256)
		return false;

	Tile* const tile = Tile::tiles[item->id];
	if (!tile)
		return false;

	// Special stone/sand cases
	if (	tile == Tile::lapisBlock
		||	tile == Tile::furnace
		||	tile == Tile::stonecutterBench)
		return false;

	if (tile == Tile::stoneSlabHalf && ins.getAuxValue() == StoneSlabTile::WOOD_SLAB)
		return false;

	// Return everything stone or sand
	return (tile->material == Material::stone || tile->material == Material::sand);
}

}