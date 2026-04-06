#include "HatchetItem.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"


HatchetItem::HatchetItem( int id, const Tier& tier ) :   super(id, 3, tier)
{
	TileList d;
	d.push_back(Tile::wood);
	d.push_back(Tile::bookshelf);
	d.push_back(Tile::treeTrunk);
	d.push_back(Tile::chest);
	d.push_back(Tile::stoneSlab);
	d.push_back(Tile::stoneSlabHalf);
	//d.push_back(Tile::pumpkin);
	//d.push_back(Tile::litPumpkin);

	setTiles(d);
}

float HatchetItem::getDestroySpeed( ItemInstance* itemInstance, Tile* tile )
{
	if (tile != NULL && tile->material == Material::wood) {
		return speed;
	}
	return super::getDestroySpeed(itemInstance, tile);
}
