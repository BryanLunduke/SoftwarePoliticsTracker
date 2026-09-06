#include "Sheep.h"
#include "../../item/DyePowderItem.h"
#include "../../level/tile/LevelEvent.h"

const float Sheep::COLOR[][3] = {
    { 1.0f, 1.0f, 1.0f }, // white
    { 0.95f, 0.7f, 0.2f }, // orange
    { 0.9f, 0.5f, 0.85f }, // magenta
    { 0.6f, 0.7f, 0.95f }, // light blue
    { 0.9f, 0.9f, 0.2f }, // yellow
    { 0.5f, 0.8f, 0.1f }, // light green
    { 0.95f, 0.7f, 0.8f }, // pink
    { 0.3f, 0.3f, 0.3f }, // gray
    { 0.6f, 0.6f, 0.6f }, // silver
    { 0.3f, 0.6f, 0.7f }, // cyan
    { 0.7f, 0.4f, 0.9f }, // purple
    { 0.2f, 0.4f, 0.8f }, // blue
    { 0.5f, 0.4f, 0.3f }, // brown
    { 0.4f, 0.5f, 0.2f }, // green
    { 0.8f, 0.3f, 0.3f }, // red
    { 0.1f, 0.1f, 0.1f }, // black
};
const int Sheep::NumColors = sizeof(Sheep::COLOR) / sizeof(Sheep*);


Sheep::Sheep( Level* level )
:   super(level),
	eatAnimationTick(0)
{
	entityRendererId = ER_SHEEP_RENDERER;

	this->textureName = "mob/sheep.png";
	this->setSize(0.9f, 1.3f);

	// sheared and color share a char
	entityData.define(DATA_WOOL_ID, (SynchedEntityData::TypeChar) 0);
}

int Sheep::getMaxHealth()
{
	return 8;
}

void Sheep::aiStep()
{
	super::aiStep();

	if (eatAnimationTick > 0) {
		eatAnimationTick--;
	}
}

void Sheep::handleEntityEvent( char id )
{
	if (id == EntityEvent::EAT_GRASS) {
		eatAnimationTick = EAT_ANIMATION_TICKS;
	} else {
		super::handleEntityEvent(id);
	}
}

float Sheep::getHeadEatPositionScale( float a )
{
	if (eatAnimationTick <= 0) {
		return 0;
	}
	if (eatAnimationTick >= 4 && eatAnimationTick <= (EAT_ANIMATION_TICKS - 4)) {
		return 1;
	}
	if (eatAnimationTick < 4) {
		return ((float) eatAnimationTick - a) / 4.f;
	}
	return -((float) (eatAnimationTick - EAT_ANIMATION_TICKS) - a) / 4.f;
}

float Sheep::getHeadEatAngleScale( float a )
{
	if (eatAnimationTick > 4 && eatAnimationTick <= (EAT_ANIMATION_TICKS - 4)) {
		float scale = ((float) (eatAnimationTick - 4) - a) / (float) (EAT_ANIMATION_TICKS - 8);
		return Mth::PI * .20f + Mth::PI * .07f * Mth::sin(scale * 28.7f);
	}
	if (eatAnimationTick > 0) {
		return Mth::PI * .20f;
	}
	return ((xRot / (float) (180 / Mth::PI)));
}

bool Sheep::interact( Player* player )
{
	ItemInstance* item = player->inventory->getSelected();
	if (item && item->id == ((Item*)Item::shears)->id && !isSheared() && !isBaby()) {
		if (!level->isClientSide) {
			setSheared(true);
			int count = 1 + random.nextInt(3);
			for (int i = 0; i < count; i++) {
				ItemEntity* ie = spawnAtLocation(new ItemInstance(Tile::cloth->id, 1, getColor()), 1);
				ie->yd += random.nextFloat() * 0.05f;
				ie->xd += (random.nextFloat() - random.nextFloat()) * 0.1f;
				ie->zd += (random.nextFloat() - random.nextFloat()) * 0.1f;
			}
		}
		item->hurt(1);
	}

	return super::interact(player);
}

void Sheep::addAdditonalSaveData( CompoundTag* tag )
{
	super::addAdditonalSaveData(tag);
	tag->putBoolean("Sheared", isSheared());
	tag->putByte("Color", (char) getColor());
}

void Sheep::readAdditionalSaveData( CompoundTag* tag )
{
	super::readAdditionalSaveData(tag);
	setSheared(tag->getBoolean("Sheared"));
	setColor((int) tag->getByte("Color"));
}

int Sheep::getColor() const
{
	return (int) (entityData.getByte(DATA_WOOL_ID) & 0x0f);
}

void Sheep::setColor( int color )
{
	char current = entityData.getByte(DATA_WOOL_ID);
	entityData.set(DATA_WOOL_ID, (SynchedEntityData::TypeChar) ((current & 0xf0) | (color & 0x0f)));
}

bool Sheep::isSheared() const
{
	return (entityData.getByte(DATA_WOOL_ID) & 0x10) != 0;
}

void Sheep::setSheared( bool value )
{
	char current = entityData.getByte(DATA_WOOL_ID);
	if (value) {
		entityData.set(DATA_WOOL_ID, (SynchedEntityData::TypeChar) (current | 0x10));
	} else {
		entityData.set(DATA_WOOL_ID, (SynchedEntityData::TypeChar) (current & ~0x10));
	}
}

int Sheep::getSheepColor( Random* random )
{
    int nextInt = random->nextInt(100);
    if (nextInt < 5)
        return 15 - DyePowderItem::BLACK;
    if (nextInt < 10)
        return 15 - DyePowderItem::GRAY;
    if (nextInt < 15)
        return 15 - DyePowderItem::SILVER;
    if (nextInt < 18)
        return 15 - DyePowderItem::BROWN;

	if (random->nextInt(500) == 0)
		return 15 - DyePowderItem::PINK;

	return 0; // white
}

int Sheep::getEntityTypeId() const
{
	return MobTypes::Sheep;
}

void Sheep::dropDeathLoot(/* bool wasKilledByPlayer, int playerBonusLevel*/ )
{
	if (!isSheared()) {
		// killing a non-sheared sheep will drop a single block of cloth
		spawnAtLocation(new ItemInstance(Tile::cloth->id, 1, getColor()), 0);
	}
}

int Sheep::getDeathLoot()
{
	return Tile::cloth->id;
}

void Sheep::jumpFromGround()
{
	if (eatAnimationTick <= 0) {
		super::jumpFromGround();
	}
}

void Sheep::updateAi()
{
	super::updateAi();

	if (!isPathFinding() && eatAnimationTick <= 0 && ((isBaby() && random.nextInt(50) == 0) || random.nextInt(1000) == 0)) {

		int xx = Mth::floor(x);
		int yy = Mth::floor(y);
		int zz = Mth::floor(z);

		if (/*(level->getTile(xx, yy, zz) == Tile::tallgrass->id && level->getData(xx, yy, zz) == TallGrass::TALL_GRASS) || */ level->getTile(xx, yy - 1, zz) == ((Tile*)Tile::grass)->id) {
			eatAnimationTick = EAT_ANIMATION_TICKS;
			level->broadcastEntityEvent(this, EntityEvent::EAT_GRASS);
		}
	} else if (eatAnimationTick == 4) {
		int xx = Mth::floor(x);
		int yy = Mth::floor(y);
		int zz = Mth::floor(z);

		bool ate = false;
		/*            if (level->getTile(xx, yy, zz) == Tile::tallgrass->id) {
		level->levelEvent(LevelEvent::PARTICLES_DESTROY_BLOCK, xx, yy, zz, Tile::tallgrass->id + TallGrass::TALL_GRASS * 256);
		level->setTile(xx, yy, zz, 0);
		ate = true;
		} else */if (level->getTile(xx, yy - 1, zz) == ((Tile*)Tile::grass)->id) {
			level->levelEvent(NULL, LevelEvent::PARTICLES_DESTROY_BLOCK, xx, yy - 1, zz, ((Tile*)Tile::grass)->id);
			level->setTile(xx, yy - 1, zz, Tile::dirt->id);
			ate = true;
		}
		if (ate) {
			setSheared(false);
			if (isBaby()) {
				// remove a minute from aging
				int age = getAge() + SharedConstants::TicksPerSecond * 60;
				if (age > 0) {
					age = 0;
				}
				setAge(age);
			}
		}
	}
}

bool Sheep::shouldHoldGround()
{
	return eatAnimationTick > 0;
}

const char* Sheep::getAmbientSound()
{
	return "mob.sheep";
}

std::string Sheep::getHurtSound()
{
	return "mob.sheep";
}

std::string Sheep::getDeathSound()
{
	return "mob.sheep";
}
