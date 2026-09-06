#include "FlyingMob.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"
#include "../../util/Mth.h"


FlyingMob::FlyingMob( Level* level )
:   super(level)
{
}

void FlyingMob::travel( float xa, float ya )
{
	if (isInWater()) {
		moveRelative(xa, ya, 0.02f);
		move(xd, yd, zd);

		xd *= 0.80f;
		yd *= 0.80f;
		zd *= 0.80f;
	} else if (isInLava()) {
		moveRelative(xa, ya, 0.02f);
		move(xd, yd, zd);
		xd *= 0.50f;
		yd *= 0.50f;
		zd *= 0.50f;
	} else {
		float friction = 0.91f;
		if (onGround) {
			friction = 0.6f * 0.91f;
			int t = level->getTile(Mth::floor(x), Mth::floor(bb.y0 - 0.5f), Mth::floor(z));
			if (t > 0) {
				friction = Tile::tiles[t]->friction * 0.91f;
			}
		}

		float friction2 = (0.6f * 0.6f * 0.91f * 0.91f * 0.6f * 0.91f) / (friction * friction * friction);
		moveRelative(xa, ya, (onGround ? 0.1f * friction2 : 0.02f));

		friction = 0.91f;
		if (onGround) {
			friction = 0.6f * 0.91f;
			int t = level->getTile(Mth::floor(x), Mth::floor(bb.y0 - 0.5f), Mth::floor(z));
			if (t > 0) {
				friction = Tile::tiles[t]->friction * 0.91f;
			}
		}

		move(xd, yd, zd);

		xd *= friction;
		yd *= friction;
		zd *= friction;
	}
	walkAnimSpeedO = walkAnimSpeed;
	float xxd = x - xo;
	float zzd = z - zo;
	float wst = Mth::sqrt(xxd * xxd + zzd * zzd) * 4;
	if (wst > 1) wst = 1;
	walkAnimSpeed += (wst - walkAnimSpeed) * 0.4f;
	walkAnimPos += walkAnimSpeed;
}

bool FlyingMob::onLadder()
{
	return false;
}

void FlyingMob::causeFallDamage( float distance )
{
}
