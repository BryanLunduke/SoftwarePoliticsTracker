#include "HangingEntity.h"
#include "../Direction.h"
#include "../level/Level.h"
#include "../level/material/Material.h"
HangingEntity::HangingEntity( Level* level )
	: super(level) {
	init();
}

HangingEntity::HangingEntity( Level* level, int xTile, int yTile, int zTile, int dir )
	: super(level), xTile(xTile), yTile(yTile), zTile(zTile) {
	init();
}
void HangingEntity::setPosition( int x, int y, int z ) {
	xTile = x;
	yTile = y;
	zTile = z;
}
void HangingEntity::init() {
	heightOffset = 0;
	setSize(0.5f, 0.5f);
	dir = 0;
	checkInterval = 0;
}

void HangingEntity::setDir( int dir ) {
	//printf("HangingEntity dir: %d\n", dir);
	this->dir = dir;
	yRotO = yRot = float(dir * 90);

	float w = float(getWidth());
	float h = float(getHeight());
	float d = float(getWidth());

	if (dir == Direction::NORTH || dir == Direction::SOUTH) {
		d = 2.0f;
		yRot = yRotO = float(Direction::DIRECTION_OPPOSITE[dir] * 90);
	} else {
		w = 2.0f;
	}

	w /= 32.0f;
	h /= 32.0f;
	d /= 32.0f;

	float x = xTile + 0.5f;
	float y = yTile + 0.5f;
	float z = zTile + 0.5f;

	float offset = 0.5f + 1.0f / 16.0f;

	if (dir == Direction::NORTH) z -= offset;
	if (dir == Direction::WEST) x -= offset;
	if (dir == Direction::SOUTH) z += offset;
	if (dir == Direction::EAST) x += offset;

	if (dir == Direction::NORTH) x -= offs(getWidth());
	if (dir == Direction::WEST) z += offs(getWidth());
	if (dir == Direction::SOUTH) x += offs(getWidth());
	if (dir == Direction::EAST) z -= offs(getWidth());
	y += offs(getHeight());

	setPos(x, y, z);

	float ss = -(0.5f / 16.0f);
	bb.set(x - w - ss, y - h - ss, z - d - ss, x + w + ss, y + h + ss, z + d + ss);
}
float HangingEntity::offs( int w ) {
	if(w == 32) return 0.5f;
	if(w == 64) return 0.5f;
	return 0;
}

void HangingEntity::tick() {
	if(checkInterval++ == 20 * 5 && !level->isClientSide) {
		checkInterval = 0;
		if(!removed && !survives()) {
			remove();
			dropItem();
		}
	}
}

bool HangingEntity::survives() {
	if (!level->getCubes(this, bb).empty()) {
		return false;
	} else {
		int ws = Mth::Max(1, getWidth() / 16);
		int hs = Mth::Max(1, getHeight() / 16);

		int xt = xTile;
		int yt = yTile;
		int zt = zTile;
		if (dir == Direction::NORTH) xt = Mth::floor(x - getWidth() / 32.0f);
		if (dir == Direction::WEST) zt = Mth::floor(z - getWidth() / 32.0f);
		if (dir == Direction::SOUTH) xt = Mth::floor(x - getWidth() / 32.0f);
		if (dir == Direction::EAST) zt = Mth::floor(z - getWidth() / 32.0f);
		yt = Mth::floor(y - getHeight() / 32.0f);

		for (int ss = 0; ss < ws; ++ss) {
			for (int yy = 0; yy < hs; ++yy) {
				const Material* m;
				if (dir == Direction::NORTH || dir == Direction::SOUTH) {
					m = level->getMaterial(xt + ss, yt + yy, zTile);
				} else {
					m = level->getMaterial(xTile, yt + yy, zt + ss);
				}
				if (!m->isSolid())
					return false;
			}
			EntityList entities = level->getEntities(this, bb);
			for(EntityList::iterator ei = entities.begin(); ei != entities.end(); ++ei) {
				Entity* entity = *(ei);
				if(entity->isHangingEntity())
					return false;
			}
		}
	}
	return true;
}

bool HangingEntity::isHangingEntity() {
	return true;
}

bool HangingEntity::isPickable() {
	return true;
}

bool HangingEntity::interact(Player* player) {
	if(!removed && !level->isClientSide) {
		if(player != NULL
			&& player->inventory != NULL
			&& player->inventory->getSelected() != NULL
			&& player->inventory->getSelected()->id == Item::bow->id)
			return false;
		remove();
		markHurt();
		if(player != NULL && !player->abilities.instabuild)
			dropItem();
		return true;
	} else {
		return !removed;
	}
}

void HangingEntity::move( float xa, float ya, float za ) {
	if(!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
		dropItem();
		remove();
	}
}

void HangingEntity::push( float xa, float ya, float za ) {
	if (!level->isClientSide && !removed && (xa * xa + ya * ya + za * za) > 0) {
		dropItem();
		remove();
	}
}

void HangingEntity::addAdditonalSaveData( CompoundTag* tag ) {
	tag->putByte("Direction",  (char) dir);
	tag->putInt("TileX", xTile);
	tag->putInt("TileY", yTile);
	tag->putInt("TileZ", zTile);

	// Back compat
	switch (dir) {
	case Direction::NORTH:
		tag->putByte("Dir", char(0));
		break;
	case Direction::WEST:
		tag->putByte("Dir", char(1));
		break;
	case Direction::SOUTH:
		tag->putByte("Dir", char(2));
		break;
	case Direction::EAST:
		tag->putByte("Dir", char(3));
		break;
	}
}

void HangingEntity::readAdditionalSaveData( CompoundTag* tag ) {
	if (tag->contains("Direction")) {
		dir = tag->getByte("Direction");
	} else {
		switch (tag->getByte("Dir")) {
		case 0:
			dir = Direction::NORTH;
			break;
		case 1:
			dir = Direction::WEST;
			break;
		case 2:
			dir = Direction::SOUTH;
			break;
		case 3:
			dir = Direction::EAST;
			break;
		}
	}
	xTile = tag->getInt("TileX");
	yTile = tag->getInt("TileY");
	zTile = tag->getInt("TileZ");
	setDir(dir);
}

float HangingEntity::getBrightness( float a ) {
	int xTile = Mth::floor(x);
	int zTile = Mth::floor(z);
	//if (dir == Direction::NORTH) xTile--;
	//if (dir == Direction::WEST) zTile++;
	//if (dir == Direction::SOUTH) xTile++;
	//if (dir == Direction::EAST) zTile--;
	if (level->hasChunkAt(xTile, 0, zTile)) {
		int yTile = Mth::floor(y);
		return level->getBrightness(xTile, yTile, zTile);
	}
	return 0;
}

bool HangingEntity::hurt( Entity* source, int damage ) {
	if(!removed && !level->isClientSide) {
		remove();
		markHurt();
		Player* player = Player::asPlayer(source);
		if(player != NULL && player->abilities.instabuild) {
			return true;
		}
		dropItem();
	}
	return true;
}
