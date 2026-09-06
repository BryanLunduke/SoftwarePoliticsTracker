#include "Arrow.h"
#include "../Mob.h"
#include "../player/Player.h"
#include "../../item/Item.h"
#include "../../item/ItemInstance.h"
#include "../../level/Level.h"
#include "../../level/tile/Tile.h"
#include "../../../util/Mth.h"

#include "../../../nbt/CompoundTag.h"

const float Arrow::ARROW_BASE_DAMAGE = 2.0f;

Arrow::Arrow( Level* level )
:   super(level),
	playerArrow(false),
	ownerId(0)
{
	_init();
}

Arrow::Arrow( Level* level, float x, float y, float z )
:   super(level),
	playerArrow(false),
	ownerId(0)
{
	_init();

	this->setPos(x, y, z);
	this->heightOffset = 0;
}

Arrow::Arrow( Level* level, Mob* mob, float power )
:   super(level),
	ownerId(mob->entityId),
	playerArrow(mob->isPlayer())
{
	_init();

	moveTo(mob->x, mob->y + mob->getHeadHeight(), mob->z, mob->yRot, mob->xRot);

	const float dx = Mth::cos(yRot * Mth::DEGRAD);
	const float dz = Mth::sin(yRot * Mth::DEGRAD);
	x -= dx * 0.16f;
	y -= 0.1f;
	z -= dz * 0.16f;
	this->setPos(x, y, z);
	this->heightOffset = 0;

	const float dxx = Mth::cos(xRot / 180 * Mth::PI);
	xd = -dz * dxx;
	zd = dx * dxx;
	yd = -Mth::sin(xRot / 180 * Mth::PI);

	shoot(xd, yd, zd, power * 1.5f, 1);
}

void Arrow::_init()
{
	entityRendererId = ER_ARROW_RENDERER;
	setSize(0.5f, 0.5f);

	shakeTime = 0;
	critArrow = false;

	xTile = -1;
	yTile = -1;
	zTile = -1;
	lastTile = 0;
	lastData = 0;
	inGround = false;
	life = 0;
	flightTime = 0;
}

void Arrow::shoot( float xd, float yd, float zd, float pow, float uncertainty )
{
	float dist = Mth::sqrt(xd * xd + yd * yd + zd * zd);

	xd /= dist;
	yd /= dist;
	zd /= dist;

	const float radius = 0.0075f * uncertainty;
	xd += sharedRandom.nextGaussian() * radius;
	yd += sharedRandom.nextGaussian() * radius;
	zd += sharedRandom.nextGaussian() * radius;

	xd *= pow;
	yd *= pow;
	zd *= pow;

	this->xd = xd;
	this->yd = yd;
	this->zd = zd;

	float sd = (float) Mth::sqrt(xd * xd + zd * zd);

	yRotO = this->yRot = (float) (std::atan2(xd, zd) * Mth::RADDEG);
	xRotO = this->xRot = (float) (std::atan2(yd, sd) * Mth::RADDEG);
	life = 0;
}

void Arrow::lerpMotion( float xd, float yd, float zd )
{
	this->xd = xd;
	this->yd = yd;
	this->zd = zd;
	if (xRotO == 0 && yRotO == 0) {
		float sd = (float) Mth::sqrt(xd * xd + zd * zd);
		yRotO = this->yRot = (float) (std::atan2(xd, zd) * Mth::RADDEG);
		xRotO = this->xRot = (float) (std::atan2(yd, sd) * Mth::RADDEG);
		moveTo(x, y, z, yRot, xRot);
		life = 0;
	}
}

void Arrow::tick()
{
	super::tick();

	if (xRotO == 0 && yRotO == 0) {
		float sd = (float) Mth::sqrt(xd * xd + zd * zd);
		yRotO = this->yRot = (float) (std::atan2(xd, zd) * Mth::RADDEG);
		xRotO = this->xRot = (float) (std::atan2(yd, sd) * Mth::RADDEG);
	}


	{
		int t = level->getTile(xTile, yTile, zTile);
		if (t > 0) {
			Tile::tiles[t]->updateShape(level, xTile, yTile, zTile);
			AABB* aabb = Tile::tiles[t]->getAABB(level, xTile, yTile, zTile);
			if (aabb != NULL && aabb->contains(Vec3(x, y, z))) {
				inGround = true;
			}
		}
	}

	if (shakeTime > 0) shakeTime--;

	if (inGround) {
		// xd = 0;
		// yd = 0;
		// zd = 0;
		int tile = level->getTile(xTile, yTile, zTile);
		int data = level->getData(xTile, yTile, zTile);
		if (tile != lastTile || data != lastData) {
			inGround = false;

			xd *= sharedRandom.nextFloat() * 0.2f;
			yd *= sharedRandom.nextFloat() * 0.2f;
			zd *= sharedRandom.nextFloat() * 0.2f;
			life = 0;
			flightTime = 0;
			return;
		} else {
			if (++life == 60 * SharedConstants::TicksPerSecond) remove();
			return;
		}
	} else {
		flightTime++;
	}

	Vec3 from(x, y, z);
	Vec3 to(x + xd, y + yd, z + zd);
	HitResult res = level->clip(from, to, false, true);

	from.set(x, y, z);
	to.set(x + xd, y + yd, z + zd);
	if (res.isHit()) {
		to.set(res.pos.x, res.pos.y, res.pos.z);
	}
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

	if (hitEntity != NULL) {
		res = HitResult(hitEntity);
	}

	if (res.isHit()) {
		if (res.type == ENTITY) {
			float pow = Mth::sqrt(xd * xd + yd * yd + zd * zd);
			int dmg = (int) std::ceil(pow * ARROW_BASE_DAMAGE);

			if (critArrow) dmg += sharedRandom.nextInt(dmg / 2 + 2);

			//@todo
			//DamageSource damageSource = NULL;
			//if (owner == NULL) {
			//    damageSource = DamageSource.arrow(this, this);
			//} else {
			//    damageSource = DamageSource.arrow(this, owner);
			//}

			//if (res.entity->hurt(damageSource, dmg)) {
			//    if (res.entity instanceof Mob) {

			if (res.entity->hurt(this, dmg)) {
				if (res.entity->isMob()) {
					((Mob*) res.entity)->arrowCount++;
				}
				level->playSound(this, "random.bowhit", 1.0f, 1.2f / (sharedRandom.nextFloat() * 0.2f + 0.9f));
				remove();
			} else {
				xd *= -0.1f;
				yd *= -0.1f;
				zd *= -0.1f;
				yRot += 180;
				yRotO += 180;
				flightTime = 0;
			}
		} else {
			xTile = res.x;
			yTile = res.y;
			zTile = res.z;
			lastTile = level->getTile(xTile, yTile, zTile);
			lastData = level->getData(xTile, yTile, zTile);
			xd = (float) (res.pos.x - x);
			yd = (float) (res.pos.y - y);
			zd = (float) (res.pos.z - z);
			float dd = Mth::sqrt(xd * xd + yd * yd + zd * zd);
			x -= (xd / dd) * 0.05f;
			y -= (yd / dd) * 0.05f;
			z -= (zd / dd) * 0.05f;

			level->playSound(this, "random.bowhit", 1.0f, 1.2f / (sharedRandom.nextFloat() * 0.2f + 0.9f));
			inGround = true;
			shakeTime = 7;
			critArrow = false;
		}
	}


	if (critArrow) {
		for (int i = 0; i < 4; i++) {
			level->addParticle(PARTICLETYPE(crit), x + xd * i / 4.0f, y + yd * i / 4.0f, z + zd * i / 4.0f, -xd, -yd + 0.2f, -zd);
		}
	}

	x += xd;
	y += yd;
	z += zd;

	float sd = Mth::sqrt(xd * xd + zd * zd);
	yRot = std::atan2(xd, zd) * Mth::RADDEG;
	xRot = std::atan2(yd, sd) * Mth::RADDEG;

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
	float gravity = 0.05f;

	if (isInWater()) {
		for (int i = 0; i < 4; i++) {
			float s = 1 / 4.0f;
			level->addParticle(PARTICLETYPE(bubble), x - xd * s, y - yd * s, z - zd * s, xd, yd, zd);
		}
		inertia = 0.80f;
	}

	xd *= inertia;
	yd *= inertia;
	zd *= inertia;
	yd -= gravity;

	setPos(x, y, z);
}

void Arrow::addAdditonalSaveData( CompoundTag* tag )
{
	tag->putShort("xTile", (short) xTile);
	tag->putShort("yTile", (short) yTile);
	tag->putShort("zTile", (short) zTile);
	tag->putByte("inTile", (char) lastTile);
	tag->putByte("inData", (char) lastData);
	tag->putByte("shake", (char) shakeTime);
	tag->putByte("inGround", (char) (inGround ? 1 : 0));
	tag->putBoolean("player", playerArrow);
}

void Arrow::readAdditionalSaveData( CompoundTag* tag )
{
	xTile = tag->getShort("xTile");
	yTile = tag->getShort("yTile");
	zTile = tag->getShort("zTile");
	lastTile = tag->getByte("inTile") & 0xff;
	lastData = tag->getByte("inData") & 0xff;
	shakeTime = tag->getByte("shake") & 0xff;
	inGround = tag->getByte("inGround") == 1;
	playerArrow = tag->getBoolean("player");
}

void Arrow::playerTouch( Player* player )
{
	if (level->isClientSide) return;

	if (inGround && playerArrow && shakeTime <= 0) {
		ItemInstance item(Item::arrow, 1);
	    if (player->inventory->add(&item)) {
	        level->playSound(this, "random.pop", 0.2f, ((sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.7f + 1.0f) * 2.f);
	        player->take(this, 1);
	        remove();
	    }
	}
}

float Arrow::getShadowHeightOffs() {
	return 0;
}

int Arrow::getEntityTypeId() const {
	return EntityTypes::IdArrow;
}

int Arrow::getAuxData() {
	return ownerId;
}
