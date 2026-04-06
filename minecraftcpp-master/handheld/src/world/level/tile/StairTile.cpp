#include "../Level.h"
#include "../../phys/HitResult.h"
#include "../../phys/Vec3.h"
#include "../../Facing.h"
#include "StairTile.h"

const int StairTile::DEAD_SPACES[8][2] = {
	{2, 6}, {3, 7}, {2, 3}, {6, 7},
	{0, 4}, {1, 5}, {0, 1}, {4, 5}
};

int StairTile::getPlacedOnFaceDataValue(Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ, int itemValue) {
	if (face == Facing::DOWN || (face != Facing::UP && clickY > 0.5)) {
		return itemValue | UPSIDEDOWN_BIT;
	}
	return itemValue;
}


bool StairTile::setStepShape(LevelSource* level, int x, int y, int z) {
    int data = level->getData(x, y, z);
    int dir = data & 0x3;

    float bottom = 0.5f;
    float top = 1.0f;

    if ((data & UPSIDEDOWN_BIT) != 0) {
        bottom = 0;
        top = .5f;
    }

    float west = 0;
    float east = 1;
    float north = 0;
    float south = .5f;

    bool checkInnerPiece = true;

    if (dir == DIR_EAST) {
        west = .5f;
        south = 1;

        int backTile = level->getTile(x + 1, y, z);
        int backData = level->getData(x + 1, y, z);
        if (isStairs(backTile) && ((data & UPSIDEDOWN_BIT) == (backData & UPSIDEDOWN_BIT))) {
            int backDir = backData & 0x3;
            if (backDir == DIR_NORTH && !isLockAttached(level, x, y, z + 1, data)) {
                south = .5f;
                checkInnerPiece = false;
            } else if (backDir == DIR_SOUTH && !isLockAttached(level, x, y, z - 1, data)) {
                north = .5f;
                checkInnerPiece = false;
            }
        }
    } else if (dir == DIR_WEST) {
        east = .5f;
        south = 1;

        int backTile = level->getTile(x - 1, y, z);
        int backData = level->getData(x - 1, y, z);
        if (isStairs(backTile) && ((data & UPSIDEDOWN_BIT) == (backData & UPSIDEDOWN_BIT))) {
            int backDir = backData & 0x3;
            if (backDir == DIR_NORTH && !isLockAttached(level, x, y, z + 1, data)) {
                south = .5f;
                checkInnerPiece = false;
            } else if (backDir == DIR_SOUTH && !isLockAttached(level, x, y, z - 1, data)) {
                north = .5f;
                checkInnerPiece = false;
            }
        }
    } else if (dir == DIR_SOUTH) {
        north = .5f;
        south = 1;

        int backTile = level->getTile(x, y, z + 1);
        int backData = level->getData(x, y, z + 1);
        if (isStairs(backTile) && ((data & UPSIDEDOWN_BIT) == (backData & UPSIDEDOWN_BIT))) {
            int backDir = backData & 0x3;
            if (backDir == DIR_WEST && !isLockAttached(level, x + 1, y, z, data)) {
                east = .5f;
                checkInnerPiece = false;
            } else if (backDir == DIR_EAST && !isLockAttached(level, x - 1, y, z, data)) {
                west = .5f;
                checkInnerPiece = false;
            }
        }
    } else if (dir == DIR_NORTH) {
        int backTile = level->getTile(x, y, z - 1);
        int backData = level->getData(x, y, z - 1);
        if (isStairs(backTile) && ((data & UPSIDEDOWN_BIT) == (backData & UPSIDEDOWN_BIT))) {
            int backDir = backData & 0x3;
            if (backDir == DIR_WEST && !isLockAttached(level, x + 1, y, z, data)) {
                east = .5f;
                checkInnerPiece = false;
            } else if (backDir == DIR_EAST && !isLockAttached(level, x - 1, y, z, data)) {
                west = .5f;
                checkInnerPiece = false;
            }
        }
    }

    setShape(west, bottom, north, east, top, south);
    return checkInnerPiece;
}

/*
    * This method adds an extra 1/8 block if the stairs can attach as an
    * "inner corner."
    */
bool StairTile::setInnerPieceShape(LevelSource* level, int x, int y, int z) {
    int data = level->getData(x, y, z);
    int dir = data & 0x3;

    float bottom = 0.5f;
    float top = 1.0f;

    if ((data & UPSIDEDOWN_BIT) != 0) {
        bottom = 0;
        top = .5f;
    }

    float west = 0;
    float east = .5f;
    float north = .5f;
    float south = 1.0f;

    bool hasInnerPiece = false;

    if (dir == DIR_EAST) {
        int frontTile = level->getTile(x - 1, y, z);
        int frontData = level->getData(x - 1, y, z);
        if (isStairs(frontTile) && ((data & UPSIDEDOWN_BIT) == (frontData & UPSIDEDOWN_BIT))) {
            int frontDir = frontData & 0x3;
            if (frontDir == DIR_NORTH && !isLockAttached(level, x, y, z - 1, data)) {
                north = 0;
                south = .5f;
                hasInnerPiece = true;
            } else if (frontDir == DIR_SOUTH && !isLockAttached(level, x, y, z + 1, data)) {
                north = .5f;
                south = 1;
                hasInnerPiece = true;
            }
        }
    } else if (dir == DIR_WEST) {
        int frontTile = level->getTile(x + 1, y, z);
        int frontData = level->getData(x + 1, y, z);
        if (isStairs(frontTile) && ((data & UPSIDEDOWN_BIT) == (frontData & UPSIDEDOWN_BIT))) {
            west = .5f;
            east = 1.0f;
            int frontDir = frontData & 0x3;
            if (frontDir == DIR_NORTH && !isLockAttached(level, x, y, z - 1, data)) {
                north = 0;
                south = .5f;
                hasInnerPiece = true;
            } else if (frontDir == DIR_SOUTH && !isLockAttached(level, x, y, z + 1, data)) {
                north = .5f;
                south = 1;
                hasInnerPiece = true;
            }
        }
    } else if (dir == DIR_SOUTH) {
        int frontTile = level->getTile(x, y, z - 1);
        int frontData = level->getData(x, y, z - 1);
        if (isStairs(frontTile) && ((data & UPSIDEDOWN_BIT) == (frontData & UPSIDEDOWN_BIT))) {
            north = 0;
            south = .5f;

            int frontDir = frontData & 0x3;
            if (frontDir == DIR_WEST && !isLockAttached(level, x - 1, y, z, data)) {
                hasInnerPiece = true;
            } else if (frontDir == DIR_EAST && !isLockAttached(level, x + 1, y, z, data)) {
                west = .5f;
                east = 1.0f;
                hasInnerPiece = true;
            }
        }
    } else if (dir == DIR_NORTH) {
        int frontTile = level->getTile(x, y, z + 1);
        int frontData = level->getData(x, y, z + 1);
        if (isStairs(frontTile) && ((data & UPSIDEDOWN_BIT) == (frontData & UPSIDEDOWN_BIT))) {
            int frontDir = frontData & 0x3;
            if (frontDir == DIR_WEST && !isLockAttached(level, x - 1, y, z, data)) {
                hasInnerPiece = true;
            } else if (frontDir == DIR_EAST && !isLockAttached(level, x + 1, y, z, data)) {
                west = .5f;
                east = 1.0f;
                hasInnerPiece = true;
            }
        }
    }

    if (hasInnerPiece) {
        setShape(west, bottom, north, east, top, south);
    }
    return hasInnerPiece;
}

HitResult StairTile::clip(Level* level, int xt, int yt, int zt, const Vec3& a, const Vec3& b)
{
	HitResult results[8];

	int data = level->getData(xt, yt, zt);
	int dir = data & 0x3;
	bool upsideDown = (data & UPSIDEDOWN_BIT) == UPSIDEDOWN_BIT;
	const int* deadSpaces = (const int*)&DEAD_SPACES[dir + (upsideDown ? 4 : 0)];

	isClipping = true;
	for (int i = 0; i < 8; i++) {
		clipStep = i;
		results[i] = super::clip(level, xt, yt, zt, a, b);
	}
	isClipping = false;

	results[deadSpaces[0]] = HitResult();
	results[deadSpaces[1]] = HitResult();

	HitResult* closest = NULL;
	double closestDist = 0;

	for (int r = 0; r < 8; r++) {
		HitResult& result = results[r];
		if (result.isHit()) {
			double dist = result.pos.distanceToSqr(b);

			if (dist > closestDist) {
				closest = &result;
				closestDist = dist;
			}
		}
	}
	if (closest == NULL)
	{
		return HitResult();
	}

	return *closest;
}
