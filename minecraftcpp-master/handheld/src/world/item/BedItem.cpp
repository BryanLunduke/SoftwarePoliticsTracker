#include "BedItem.h"
#include "../level/Level.h"
#include "../level/tile/BedTile.h"
#include "../entity/player/Player.h"
#include "../Direction.h"
#include "../Facing.h"
bool BedItem::useOn( ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ ) {
	if(face != Facing::UP) {
		return false;
	}
	y += 1;
	BedTile *tile = (BedTile*) Tile::bed;
	int dir = (Mth::floor(player->yRot * 4 / (360) + 0.5f)) & 3;
	int xra = 0;
	int zra = 0;

	if (dir == Direction::SOUTH) zra = 1;
	if (dir == Direction::WEST) xra = -1;
	if (dir == Direction::NORTH) zra = -1;
	if (dir == Direction::EAST) xra = 1;

	//if (!player->mayBuild(x, y, z) || !player->mayBuild(x + xra, y, z + zra)) return false;
	if (level->isEmptyTile(x, y, z) && level->isEmptyTile(x + xra, y, z + zra) && level->isSolidBlockingTile(x, y - 1, z) && level->isSolidBlockingTile(x + xra, y - 1, z + zra)) {

		level->setTileAndData(x, y, z, tile->id, dir);
		// double-check that the bed was successfully placed
		if (level->getTile(x, y, z) == tile->id) {
			level->setTileAndData(x + xra, y, z + zra, tile->id, dir + BedTile::HEAD_PIECE_DATA);
		}

		itemInstance->count--;
		return true;
	}

	return false;
}