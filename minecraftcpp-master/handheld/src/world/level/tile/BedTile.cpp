#include "BedTile.h"
#include "../Level.h"
#include "../dimension/Dimension.h"
#include "../../entity/player/Player.h"
#include "../../Facing.h"
#include "../../Direction.h"

const int BedTile::HEAD_DIRECTION_OFFSETS[4][2] = {
	{  0,  1 },
	{ -1,  0 },
	{  0, -1 },
	{  1,  0 }
};

BedTile::BedTile( int id ) : super(id, 6 + 8 * 16, Material::cloth) {
	setShape();
}

bool BedTile::use(Level* level, int x, int y, int z, Player* player) {
	if(player->isSleeping()) {
		int dimensionsMatch =  player->bedPosition.x == x  ? 1 : 0;
		dimensionsMatch += player->bedPosition.y == y ? 1 : 0;
		dimensionsMatch += player->bedPosition.z == z ? 1 : 0;
		int maxDistance = Mth::abs(player->bedPosition.x - x);
		maxDistance = Mth::Max(maxDistance, Mth::abs(player->bedPosition.y - y));
		maxDistance = Mth::Max(maxDistance, Mth::abs(player->bedPosition.z - z));
		if(dimensionsMatch >= 2 && maxDistance < 3) {
			player->stopSleepInBed(false, true, true);
			return true;
		}
	}
	if(level->isClientSide) return true;
	int data = level->getData(x, y, z);

	if(!isHeadPiece(data)) {
		int direction = DirectionalTile::getDirection(data);
		x += HEAD_DIRECTION_OFFSETS[direction][0];
		z += HEAD_DIRECTION_OFFSETS[direction][1];
		if(level->getTile(x, y, z) != id) {
			return true;
		}
		data = level->getData(x, y, z);
	}
	if(!level->dimension->mayRespawn()) {
		float xc = x + 0.5f;
		float yc = y + 0.5f;
		float zc = z + 0.5f;
		level->setTile(x, y, z, 0);
		int direction = DirectionalTile::getDirection(data);
		x += HEAD_DIRECTION_OFFSETS[direction][0];
		y += HEAD_DIRECTION_OFFSETS[direction][1];
		if(level->getTile(x, y, z) == id) {
			level->setTile(x, y, z, 0);
			xc = (xc + x + 0.5f) / 2;
			yc = (yc + y + 0.5f) / 2;
			zc = (zc + z + 0.5f) / 2;
		}
		level->explode(NULL, x + 0.5f, y + 0.5f, z + 0.5f, 5, true);
		return true;
	}
	if(isOccupied(data)) {
		Player* sleepingPlayer = NULL;
		for(PlayerList::iterator i = level->players.begin(); i != level->players.end(); ++i) {
			if((*i)->isSleeping()) {
				Pos pos = (*i)->bedPosition;
				if(pos.x == x && pos.y == y && pos.z == z) {
					sleepingPlayer = (*i);
					break;
				}
			}
		}
		if(sleepingPlayer == NULL) {
			BedTile::setOccupied(level, x, y, z, false);
		}
		else {
			sleepingPlayer->displayClientMessage("This bed is occupied"/*"tile.bed.occupied"*/);
			return true;
		}
	}
	int result = player->startSleepInBed(x, y, z);
	if(result == BedSleepingResult::OK) {
		BedTile::setOccupied(level, x, y, z, true);
		return true;
	}
	if(result == BedSleepingResult::NOT_POSSIBLE_NOW) {
		player->displayClientMessage("You can only sleep at night" /*tile.bed.noSleep"*/);
	} else if(result == BedSleepingResult::NOT_SAFE) {
		player->displayClientMessage("You may not rest now, there are monsters nearby"/*"tile.bed.notSafe"*/);
	}
	return true;
}

void BedTile::setOccupied( Level* level, int x, int y, int z, bool occupied ) {
	int data = level->getData(x, y, z);
	if(occupied) {
		data |= OCCUPIED_DATA;
	} else {
		data &= ~OCCUPIED_DATA;
	}
	level->setData(x, y, z, data);
}

int BedTile::getTexture( int face, int data ) {
	if(face == Facing::DOWN) {
		return Tile::wood->tex;
	}
	int direction = getDirection(data);
	int tileFacing = Direction::RELATIVE_DIRECTION_FACING[direction][face];
	if (isHeadPiece(data)) {
		if (tileFacing == Facing::NORTH) {
			return tex + 2 + 16;
		}
		if (tileFacing == Facing::EAST || tileFacing == Facing::WEST) {
			return tex + 1 + 16;
		}
		return tex + 1;
	} else {
		if (tileFacing == Facing::SOUTH) {
			return tex - 1 + 16;
		}
		if (tileFacing == Facing::EAST || tileFacing == Facing::WEST) {
			return tex + 16;
		}
		return tex;
	}
}

int BedTile::getRenderShape() {
	return Tile::SHAPE_BED;
}

bool BedTile::isCubeShaped() {
	return false;
}

bool BedTile::isSolidRender() {
	return false;
}

void BedTile::neighborChanged( Level* level, int x, int y, int z, int type ) {
	int data = level->getData(x, y, z);
	int direction = getDirection(data);
	if(isHeadPiece(data)) {
		if(level->getTile(x - HEAD_DIRECTION_OFFSETS[direction][0], y, z - HEAD_DIRECTION_OFFSETS[direction][1]) != id) {
			level->setTile(x, y, z, 0);
		}
	} else {
		if(level->getTile(x + HEAD_DIRECTION_OFFSETS[direction][0], y, z + HEAD_DIRECTION_OFFSETS[direction][1]) != id) {
			level->setTile(x, y, z, 0);
			if(!level->isClientSide) {
				//spawnResources(level, x, y, z, data, 1);
				popResource(level, x, y, z, ItemInstance(Item::bed));
			}
		}
	}
}

int BedTile::getResource( int data, Random* random ) {
	if(isHeadPiece(data)) {
		return 0;
	}
	return Item::bed->id;
}

bool BedTile::findStandUpPosition( Level* level, int x, int y, int z, int skipCount, Pos& position) {
	int data = level->getData(x, y, z);
	int direction = DirectionalTile::getDirection(data);
	for(int step = 0; step <= 1; ++step) {
		int startX = x - BedTile::HEAD_DIRECTION_OFFSETS[direction][0] * step - 1;
		int startZ = z - BedTile::HEAD_DIRECTION_OFFSETS[direction][1] * step - 1;
		int endX = startX + 2;
		int endZ = startZ + 2;
		for(int standX = startX; standX <= endX; ++standX) {
			for (int standZ = startZ; standZ <= endZ; ++standZ) {
				if (level->isSolidBlockingTile(standX, y - 1, standZ) && level->isEmptyTile(standX, y, standZ) && level->isEmptyTile(standX, y + 1, standZ)) {
					if (skipCount > 0) {
						skipCount--;
						continue;
					}
					position = Pos(standX, y, standZ);
					return true;
				}
			}
		}
	}
	return false;
}

void BedTile::spawnResources( Level* level, int x, int y, int z, int data, float odds ) {
	if(!isHeadPiece(data)) {
		//super::spawnResources(level, x, y, z, data, odds);
		popResource(level, x, y, z, ItemInstance(Item::bed));
	}
}

void BedTile::updateShape( LevelSource* level, int x, int y, int z ) {
	setShape();
}

int BedTile::getRenderLayer() {
	return Tile::RENDERLAYER_ALPHATEST;
}
void BedTile::setShape() {
	super::setShape(0, 0, 0, 1, 9.0f / 16.0f, 1);
}

bool BedTile::isHeadPiece( int data ) {
		return (data & HEAD_PIECE_DATA) != 0;
}

bool BedTile::isOccupied( int data ) {
	return (data & OCCUPIED_DATA) != 0;
}
