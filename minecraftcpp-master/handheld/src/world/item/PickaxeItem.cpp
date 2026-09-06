#include "PickaxeItem.h"
#include "../level/material/Material.h"
#include "../level/tile/Tile.h"

PickaxeItem::PickaxeItem( int id, const Tier& tier ) :   super(id, 2, tier)
{
	TileList d;
	d.push_back(Tile::stoneBrick);
	d.push_back(Tile::stoneSlab);
	d.push_back(Tile::stoneSlabHalf);
	d.push_back(Tile::rock);
	d.push_back(Tile::sandStone);
	d.push_back(Tile::mossStone);
	d.push_back(Tile::ironOre);
	d.push_back(Tile::ironBlock);
	d.push_back(Tile::coalOre);
	d.push_back(Tile::goldBlock);
	d.push_back(Tile::goldOre);
	d.push_back(Tile::emeraldOre);
	d.push_back(Tile::emeraldBlock);
	d.push_back(Tile::ice);
	//d.push_back(Tile::hellRock);
	d.push_back(Tile::lapisOre);
	d.push_back(Tile::lapisBlock);
	d.push_back(Tile::redStoneOre);
	d.push_back(Tile::redStoneOre_lit);
	//d.push_back(Tile::rail);
	//d.push_back(Tile::detectorRail);
	//d.push_back(Tile::goldenRail);

	setTiles(d);
}

bool PickaxeItem::canDestroySpecial( const Tile* tile ) const
{
	if (tile == Tile::obsidian) return tier.getLevel() == 3;
	if (tile == Tile::emeraldBlock || tile == Tile::emeraldOre) return tier.getLevel() >= 2;
	if (tile == Tile::goldBlock || tile == Tile::goldOre) return tier.getLevel() >= 2;
	if (tile == Tile::ironBlock || tile == Tile::ironOre) return tier.getLevel() >= 1;
	if (tile == Tile::lapisBlock || tile == Tile::lapisOre) return tier.getLevel() >= 1;
	if (tile == Tile::redStoneOre || tile == Tile::redStoneOre_lit) return tier.getLevel() >= 2;
	if (tile->material == Material::stone) return true;
	if (tile->material == Material::metal) return true;
	return false;
}

float PickaxeItem::getDestroySpeed( ItemInstance* itemInstance, Tile* tile )
{
	if (tile != NULL && (tile->material == Material::metal || tile->material == Material::stone)) {
		return speed;
	}
	return super::getDestroySpeed(itemInstance, tile);
}
