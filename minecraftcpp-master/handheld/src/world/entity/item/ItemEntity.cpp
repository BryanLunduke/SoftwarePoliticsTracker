#include "ItemEntity.h"
#include "../../entity/player/Player.h"
#include "../../item/ItemInstance.h"
#include "../../level/Level.h"
#include "../../level/material/Material.h"
#include "../../level/tile/Tile.h"
#include "../../../util/Mth.h"
#include "../../../nbt/CompoundTag.h"

const int ItemEntity::LIFETIME = 5 * 60 * SharedConstants::TicksPerSecond; // Five minutes, changed in 0.3.3!

ItemEntity::ItemEntity( Level* level, float x, float y, float z, const ItemInstance& item )
:	super(level),
	item(item),
	health(5),
	age(0),
	tickCount(0),
	throwTime(0),
	lifeTime(LIFETIME),
	bobOffs((float) (Mth::random() * Mth::PI * 2))
{
	entityRendererId = ER_ITEM_RENDERER;

	setSize(0.25f, 0.25f);
	heightOffset = bbHeight / 2.0f;
	setPos(x, y, z);

	yRot = (float) (Mth::random() * 360);

	xd = (float) (Mth::random() * 0.2f - 0.1f);
	yd = +0.2f;
	zd = (float) (Mth::random() * 0.2f - 0.1f);
	makeStepSound = false;
}

ItemEntity::ItemEntity( Level* level )
:	super(level),
	health(5),
	age(0),
	tickCount(0),
	throwTime(0),
	lifeTime(LIFETIME),
	bobOffs((float) (Mth::random() * Mth::PI * 2))
{
	entityRendererId = ER_ITEM_RENDERER;

	setSize(0.25f, 0.25f);
	heightOffset = bbHeight / 2.0f;
}

ItemEntity::~ItemEntity()
{
}


void ItemEntity::tick()
{
	super::tick();
	if (throwTime > 0) throwTime--;
	xo = x;
	yo = y;
	zo = z;

	yd -= 0.04f;
	if (level->getMaterial(Mth::floor(x), Mth::floor(y), Mth::floor(z)) == Material::lava) {
		yd = 0.2f;
		xd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.2f;
		zd = (sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.2f;
		level->playSound(this, "random.fizz", 0.4f, 2.0f + sharedRandom.nextFloat() * 0.4f);
	}
	checkInTile(x, y, z);
	move(xd, yd, zd);

	float friction = 0.98f;
	if (onGround) {
		friction = 0.6f * 0.98f;
		int t = level->getTile(Mth::floor(x), Mth::floor(bb.y0) - 1, Mth::floor(z));
		if (t > 0) {
			friction = Tile::tiles[t]->friction * 0.98f;
		}
	}

	xd *= friction;
	yd *= 0.98f;
	zd *= friction;

	if (onGround) {
		yd *= -0.5f;
	}

	tickCount++;
	age++;
	if (age >= lifeTime) {
		remove();
	}
}

bool ItemEntity::isInWater()
{
	return level->checkAndHandleWater(bb, Material::water, this);
}

bool ItemEntity::hurt( Entity* source, int damage )
{
	markHurt();
	health -= damage;
	if (health <= 0) {
		remove();
	}
	return false;
}

void ItemEntity::playerTouch( Player* player )
{
	if (level->isClientSide) return;

	int orgCount = item.count;
	if (throwTime == 0 && player->isAlive() && player->inventory->add(&item)) {
	    level->playSound(this, "random.pop", 0.3f, ((sharedRandom.nextFloat() - sharedRandom.nextFloat()) * 0.7f + 1.0f) * 2.f);
	    player->take(this, orgCount);
		//if (item.count <= 0) remove(); //@todo
		remove();
	}
}

void ItemEntity::burn( int dmg )
{
	hurt(NULL, dmg);
}

bool ItemEntity::checkInTile( float x, float y, float z )
{
	int xTile = Mth::floor(x);
	int yTile = Mth::floor(y);
	int zTile = Mth::floor(z);

	float xd = x - xTile;
	float yd = y - yTile;
	float zd = z - zTile;

	if (Tile::solid[level->getTile(xTile, yTile, zTile)]) {
		bool west = !Tile::solid[level->getTile(xTile - 1, yTile, zTile)];
		bool east = !Tile::solid[level->getTile(xTile + 1, yTile, zTile)];
		bool up = !Tile::solid[level->getTile(xTile, yTile - 1, zTile)];
		bool down = !Tile::solid[level->getTile(xTile, yTile + 1, zTile)];
		bool north = !Tile::solid[level->getTile(xTile, yTile, zTile - 1)];
		bool south = !Tile::solid[level->getTile(xTile, yTile, zTile + 1)];

		int dir = -1;
		float closest = 9999;
		if (west && xd < closest) {
			closest = xd;
			dir = 0;
		}
		if (east && 1 - xd < closest) {
			closest = 1 - xd;
			dir = 1;
		}
		if (up && yd < closest) {
			closest = yd;
			dir = 2;
		}
		if (down && 1 - yd < closest) {
			closest = 1 - yd;
			dir = 3;
		}
		if (north && zd < closest) {
			closest = zd;
			dir = 4;
		}
		if (south && 1 - zd < closest) {
		//	closest = 1 - zd; //@note: not read
			dir = 5;
		}

		float speed = sharedRandom.nextFloat() * 0.2f + 0.1f;
		if (dir == 0) this->xd = -speed;
		if (dir == 1) this->xd = +speed;
		if (dir == 2) this->yd = -speed;
		if (dir == 3) this->yd = +speed;
		if (dir == 4) this->zd = -speed;
		if (dir == 5) this->zd = +speed;
	}

	return false;
}

void ItemEntity::addAdditonalSaveData(CompoundTag* entityTag) {
    entityTag->putShort("Health", (char) health);
    entityTag->putShort("Age", (short) age);
    entityTag->putCompound("Item", item.save(new CompoundTag()));
}

void ItemEntity::readAdditionalSaveData(CompoundTag* tag) {
    health = tag->getShort("Health") & 0xff;
    age = tag->getShort("Age");
    CompoundTag* itemTag = tag->getCompound("Item");
	item.load(itemTag);
}

bool ItemEntity::isItemEntity() {
	return true;
}

int ItemEntity::getEntityTypeId() const {
	return EntityTypes::IdItemEntity;
}

int ItemEntity::getLifeTime() const {
	return lifeTime;
}
