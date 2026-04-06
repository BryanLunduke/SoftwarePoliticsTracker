#include "DyePowderItem.h"
#include "../entity/Mob.h"
#include "../entity/animal/Sheep.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../level/tile/Sapling.h"
#include "../level/tile/GrassTile.h"
#include "../../util/Mth.h"
#include "../level/tile/ClothTile.h"
#include "../level/tile/CropTile.h"
#include "../level/tile/StemTile.h"

const std::string DyePowderItem::COLOR_DESCS[] = {
	"black", "red", "green", "brown", "blue", "purple", "cyan", "silver", "gray", "pink", "lime", "yellow", "lightBlue", "magenta", "orange", "white"
};

const int DyePowderItem::COLOR_RGB[] = {
	0x1e1b1b, 0xb3312c, 0x3b511a, 0x51301a, 0x253192, 0x7b2fbe, 0x287697, 0x287697, 0x434343, 0xd88198, 0x41cd34, 0xdecf2a, 0x6689d3, 0xc354cd, 0xeb8844, 0xf0f0f0
};

DyePowderItem::DyePowderItem( int id )
:	super(id)
{
	setStackedByData(true);
	setMaxDamage(0);
}

int DyePowderItem::getIcon( int itemAuxValue )
{
	int colorValue = Mth::clamp(itemAuxValue, 0, 15);
	return icon + (colorValue % 8) * ICON_COLUMNS + (colorValue / 8);
}

std::string DyePowderItem::getDescriptionId( const ItemInstance* itemInstance ) const
{
	int colorValue = Mth::clamp(itemInstance->getAuxValue(), 0, 15);
	return super::getDescriptionId() + "." + COLOR_DESCS[colorValue];
}

bool DyePowderItem::useOn( ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ )
{
	//if (!player->mayBuild(x, y, z)) return false;

	if (itemInstance->getAuxValue() == WHITE) {
		// bone meal is a fertilizer, so instantly grow trees and stuff

		int tile = level->getTile(x, y, z);
		if (tile == Tile::sapling->id) {
			if (!level->isClientSide) {
				((Sapling*) Tile::sapling)->growTree(level, x, y, z, &level->random);
			}
			itemInstance->count--;
			return true;
		} /*else if (tile == Tile::mushroom1->id || tile == Tile::mushroom2->id) {
		  if (!level->isOnline) {
		  if (((Mushroom) Tile::tiles[tile]).growTree(level, x, y, z, level->random)) {
		  itemInstance.count--;
		  }
		  }
		  return true;
		  }*/ else if (tile == Tile::melonStem->id/* || tile == Tile::pumpkinStem->id*/) {
			  if (!level->isClientSide) {
				((StemTile*) Tile::tiles[tile])->growCropsToMax(level, x, y, z);
			  }
			  itemInstance->count--;
			  return true;
		  } else if (tile == Tile::crops->id) {
			  if (!level->isClientSide) {
				  ((CropTile*) Tile::crops)->growCropsToMax(level, x, y, z);
			  }
			  itemInstance->count--;
		  } else if (tile == Tile::grass->id) {
			  if (!level->isClientSide) {
				  
				  for (int j = 0; j < 32; j++) {
					  int xx = x;
					  int yy = y + 1;
					  int zz = z;
					  bool continueMainLoop = false;
					  for (int i = 0; i < j / 16; i++) {
						  xx += random.nextInt(3) - 1;
						  yy += (random.nextInt(3) - 1) * random.nextInt(3) / 2;
						  zz += random.nextInt(3) - 1;
						  if (level->getTile(xx, yy - 1, zz) != Tile::grass->id || level->isSolidBlockingTile(xx, yy, zz)) {
							  continueMainLoop = true;
							  break;
						  }
					  }
					  if(continueMainLoop)
						  continue;

					  if (level->getTile(xx, yy, zz) == 0) {
						  /*if (random.nextInt(10) != 0) {
						  level->setTileAndData(xx, yy, zz, Tile::tallgrass.id, TallGrass.TALL_GRASS);
						  } else*/ if (random.nextInt(3) != 0) {
							  level->setTile(xx, yy, zz, Tile::flower->id);
						  } else {
							  level->setTile(xx, yy, zz, Tile::rose->id);
						  }
					  }
				  }
			  }
			  itemInstance->count--;
			  return true;
		}
	}
	return false;
}

void DyePowderItem::interactEnemy( ItemInstance* itemInstance, Mob* mob )
{
	if (mob->getEntityTypeId() == MobTypes::Sheep) {
		Sheep* sheep = (Sheep*) mob;
		// convert to tile-based color value (0 is white instead of black)
		int newColor = ClothTile::getTileDataForItemAuxValue(itemInstance->getAuxValue());
		if (!sheep->isSheared() && sheep->getColor() != newColor) {
			sheep->setColor(newColor);
			itemInstance->count--;
		}
	}
}
