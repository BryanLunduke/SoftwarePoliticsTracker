#include "Pig.h"
#include "../player/Player.h"

Pig::Pig( Level* level ) :	super(level)
{
	entityRendererId = ER_PIG_RENDERER;

	textureName = "mob/pig.png";
	setSize(0.9f, 0.9f);

	//entityData.define(DATA_SADDLE_ID, (SynchedEntityData::TypeChar) 0);
}

int Pig::getEntityTypeId() const
{
	return MobTypes::Pig;
}

bool Pig::interact( Player* player )
{
	return false;
}

bool Pig::hasSaddle()
{
	return false;
	//return (entityData.getByte(DATA_SADDLE_ID) & 1) != 0;
}

void Pig::setSaddle( bool value )
{
	//if (value) {
	//    entityData.set(DATA_SADDLE_ID, (char) 1);
	//} else {
	//    entityData.set(DATA_SADDLE_ID, (char) 0);
	//}
}

const char* Pig::getAmbientSound()
{
	return "mob.pig";
}

std::string Pig::getHurtSound()
{
	return "mob.pig";
}

std::string Pig::getDeathSound()
{
	return "mob.pigdeath";
}

int Pig::getDeathLoot()
{
	//@fire
	//if (isOnFire())
	//	return Item::porkChop_cooked->id;
	return Item::porkChop_raw->id;
}

Animal* Pig::getBreedOffspring( Animal* target )
{
	return new Pig(level);
}

int Pig::getMaxHealth()
{
	return 10;
}
