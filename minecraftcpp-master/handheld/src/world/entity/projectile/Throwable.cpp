#include "Throwable.h"
#include "../player/Player.h"
#include "../../level/Level.h"
#include "../../../util/Mth.h"
#include "../../phys/HitResult.h"

Throwable::Throwable( Level* level )
:	super(level)
{
	_init();
	setSize(0.25f, 0.25f);
}

Throwable::Throwable( Level* level, Mob* mob )
:	super(level)
{
	_init();
    setSize(0.25f, 0.25f);

    ownerId = mob->entityId;

	this->moveTo(mob->x, mob->y + mob->getHeadHeight(), mob->z, mob->yRot, mob->xRot);

    const float rcos = Mth::cos(yRot / 180 * Mth::PI);
    const float rsin = Mth::sin(yRot / 180 * Mth::PI);
	x -= rcos * 0.16f;
	y -= 0.1f;
	z -= rsin * 0.16f;
	this->setPos(x, y, z);
	this->heightOffset = 0;

    if (mob->isPlayer()) {
        shoot(mob->aimDirection, getThrowPower(), 1);
    } else {
        float xd = (-rsin * Mth::cos(xRot / 180 * Mth::PI));
        float zd = ( rcos * Mth::cos(xRot / 180 * Mth::PI));
        float yd = (-Mth::sin((xRot + getThrowUpAngleOffset()) / 180 * Mth::PI));
        shoot(xd, yd, zd, getThrowPower(), 1);
    }
}

Throwable::Throwable( Level* level, float x, float y, float z )
:	super(level)
{
	_init();
    setSize(0.25f, 0.25f);

	this->setPos(x, y, z);
	this->heightOffset = 0;
}

void Throwable::_init() {
	ownerId   = 0;
	shakeTime = 0;
	inGround  = false;

	life = 0;
	flightTime = 0;
	xTile = -1;
	yTile = -1;
	zTile = -1;
	lastTile = 0;
}

bool Throwable::shouldRenderAtSqrDistance( float distance ) {
	float size = bb.getSize() * 4;
	size *= 64.0f;
	return distance < size * size;
}

float Throwable::getThrowPower() {
	return 1.5f;
}

float Throwable::getThrowUpAngleOffset() {
	return 0;
}

float Throwable::getGravity() {
	return 0.03f;
}

void Throwable::shoot(const Vec3& v, float pow, float uncertainty) {
    shoot(v.x, v.y, v.z, pow, uncertainty);
}

void Throwable::shoot( float xd, float yd, float zd, float pow, float uncertainty ) {
	float dist = Mth::sqrt(xd * xd + yd * yd + zd * zd);

    if (dist >= 0.001f) {
	    xd /= dist;
	    yd /= dist;
	    zd /= dist;
    } else {
        xd = yd = zd = 0;
    }

	xd += (sharedRandom.nextGaussian()) * 0.0075f * uncertainty;
	yd += (sharedRandom.nextGaussian()) * 0.0075f * uncertainty;
	zd += (sharedRandom.nextGaussian()) * 0.0075f * uncertainty;

	xd *= pow;
	yd *= pow;
	zd *= pow;

	this->xd = xd;
	this->yd = yd;
	this->zd = zd;

	float sd = (float) Mth::sqrt(xd * xd + zd * zd);

	yRotO = this->yRot = (float) (Mth::atan2(xd, zd) * 180 / Mth::PI);
	xRotO = this->xRot = (float) (Mth::atan2(yd, sd) * 180 / Mth::PI);
	life = 0;
}

void Throwable::lerpMotion( float xd, float yd, float zd ) {
	this->xd = xd;
	this->yd = yd;
	this->zd = zd;
	if (xRotO == 0 && yRotO == 0) {
		float sd = (float) Mth::sqrt(xd * xd + zd * zd);
		yRotO = this->yRot = (float) (Mth::atan2(xd, zd) * 180 / Mth::PI);
		xRotO = this->xRot = (float) (Mth::atan2(yd, sd) * 180 / Mth::PI);
	}
}

void Throwable::tick() {
	xOld = x;
	yOld = y;
	zOld = z;
	super::tick();

	if (shakeTime > 0) shakeTime--;

	if (inGround) {
		// xd = 0;
		// yd = 0;
		// zd = 0;
		int tile = level->getTile(xTile, yTile, zTile);
		if (tile != lastTile) {
			inGround = false;

			xd *= sharedRandom.nextFloat() * 0.2f;
			yd *= sharedRandom.nextFloat() * 0.2f;
			zd *= sharedRandom.nextFloat() * 0.2f;
			life = 0;
			flightTime = 0;
		} else {
			life++;
			if (life == 20 * 60) 
				remove();
			return;
		}
	} else {
		flightTime++;
	}

	Vec3 from(x, y, z);
	Vec3 to(x + xd, y + yd, z + zd);
	HitResult res = level->clip(from, to);

	from.set(x, y, z);
	to.set(x + xd, y + yd, z + zd);
	if (res.isHit())
		to.set(res.pos.x, res.pos.y, res.pos.z);

	if (!level->isClientSide) {
		Entity* hitEntity = NULL;
		EntityList& objects = level->getEntities(this, this->bb.expand(xd, yd, zd).grow(1, 1, 1));
		float nearest = 0;
		for (unsigned int i = 0; i < objects.size(); i++) {
			Entity* e = objects[i];
			if (!e->isPickable() || (e->entityId == ownerId && flightTime < 5)) continue;

			float rr = 0.3f;
			AABB bb = e->bb.grow(rr, rr, rr);
			HitResult p = bb.clip(from, to);
			if (p.isHit()) {
				float dd = from.distanceTo(p.pos);
				if (dd < nearest || nearest == 0) {
					hitEntity = e;
					nearest = dd;
				}
			}
		}

		if (hitEntity != NULL)
			res = HitResult(hitEntity);
	}

	if (res.isHit())
		onHit(res);

	x += xd;
	y += yd;
	z += zd;

	float sd = Mth::sqrt(xd * xd + zd * zd);
	yRot = Mth::atan2(xd, zd) * 180 / Mth::PI;
	xRot = Mth::atan2(yd, sd) * 180 / Mth::PI;

	while (xRot - xRotO < -180)
		xRotO -= 360;
	while (xRot - xRotO >= 180)
		xRotO += 360;

	while (yRot - yRotO < -180)
		yRotO -= 360;
	while (yRot - yRotO >= 180)
		yRotO += 360;

	xRot = xRotO + (xRot - xRotO) * 0.2f;
	yRot = yRotO + (yRot - yRotO) * 0.2f;


	float inertia = 0.99f;
	float gravity = getGravity();

	if (isInWater()) {
		for (int i = 0; i < 4; i++) {
			float s = 1 / 4.0f;
			level->addParticle("bubble", x - xd * s, y - yd * s, z - zd * s, xd, yd, zd);
		}
		inertia = 0.80f;
	}

	xd *= inertia;
	yd *= inertia;
	zd *= inertia;
	yd -= gravity;

	setPos(x, y, z);
}

float Throwable::getShadowHeightOffs() {
	return 0;
}

void Throwable::addAdditonalSaveData( CompoundTag* tag ) {
	tag->putShort("xTile", (short) xTile);
	tag->putShort("yTile", (short) yTile);
	tag->putShort("zTile", (short) zTile);
	tag->putByte("inTile", (char) lastTile);
	tag->putByte("shake", (char) shakeTime);
	tag->putByte("inGround", (char) (inGround ? 1 : 0));
}

void Throwable::readAdditionalSaveData( CompoundTag* tag ) {
	xTile = tag->getShort("xTile");
	yTile = tag->getShort("yTile");
	zTile = tag->getShort("zTile");
	lastTile = tag->getByte("inTile") & 0xff;
	shakeTime = tag->getByte("shake") & 0xff;
	inGround = tag->getByte("inGround") == 1;
}

int Throwable::getAuxData() {
    return ownerId;
}
