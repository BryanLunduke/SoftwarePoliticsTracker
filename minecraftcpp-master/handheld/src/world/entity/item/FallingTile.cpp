#include "FallingTile.h"
#include "../../level/Level.h"
#include "../../../util/Random.h"
#include "../../../nbt/CompoundTag.h"


FallingTile::FallingTile( Level* level )
:	Entity(level),
    tile(0),
    data(0)
{
    init();
}

FallingTile::FallingTile( Level* level, float x, float y, float z, int tile, int data)
:	Entity(level),
	tile(tile),
    data(data)
{
    init();

    setPos(x, y, z);
	xo = xOld = x;
	yo = yOld = y;
	zo = zOld = z;
}

void FallingTile::init() {
    entityRendererId = ER_FALLINGTILE_RENDERER;

	time = 0;
    blocksBuilding = true;
    setSize(0.98f, 0.98f);
    heightOffset = bbHeight / 2.0f;

    makeStepSound = false;

    xd = 0;
    yd = 0;
    zd = 0;
}

bool FallingTile::isPickable() {
	return !removed;
}

void FallingTile::tick() {
	if (tile == 0) {
		remove();
		return;
	}

	xo = x;
	yo = y;
	zo = z;
	time++;

	yd -= 0.04f;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;

	if (!level->isClientSide) {
		int xt = Mth::floor(x);
		int yt = Mth::floor(y);
		int zt = Mth::floor(z);

		if (time == 1) {
			if (level->getTile(xt, yt, zt) == tile) {
				level->setTile(xt, yt, zt, 0);
			} else {
				remove();
				return;
			}
		}

		if (onGround) {
			xd *= 0.7f;
			zd *= 0.7f;
			yd *= -0.5f;

			remove();
			if (level->mayPlace(tile, xt, yt, zt, true, 1) && level->setTileAndData(xt, yt, zt, tile, data)) {
			} else if (!level->isClientSide) {
				//spawnAtLocation(tile, 1);
			}
		} else if (time > SharedConstants::TicksPerSecond * 5 && !level->isClientSide) {
			//spawnAtLocation(tile, 1);
			remove();
		}
	}
}

float FallingTile::getShadowHeightOffs() {
	return 0;
}

Level* FallingTile::getLevel() {
	return level;
}

void FallingTile::addAdditonalSaveData( CompoundTag* tag ) {
	tag->putByte("Tile", (char) tile);
    tag->putByte("Data", (char) data);
    tag->putByte("Time", (char) time);
}

void FallingTile::readAdditionalSaveData( CompoundTag* tag ) {
	tile = tag->getByte("Tile");
    data = tag->getByte("Data");
    time = tag->getByte("Time");
}

int FallingTile::getEntityTypeId() const {
    return EntityTypes::IdFallingTile;
}
