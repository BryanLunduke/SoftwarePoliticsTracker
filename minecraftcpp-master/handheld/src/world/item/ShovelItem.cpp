#include "ShovelItem.h"
#include "../level/tile/Tile.h"

ShovelItem::ShovelItem( int id, const Tier& tier )
:   super(id, 1, tier)
{
	TileList d;
	d.push_back((Tile*)Tile::grass);
	d.push_back(Tile::dirt);
	d.push_back(Tile::sand);
	d.push_back(Tile::gravel);
	d.push_back(Tile::topSnow);
	d.push_back(Tile::snow);
	d.push_back(Tile::clay);
	d.push_back(Tile::farmland);
	
	setTiles(d);
}

bool ShovelItem::canDestroySpecial( const Tile* tile ) const {
	if (tile == Tile::topSnow) return true;
	if (tile == Tile::snow) return true;
	return false;
}
