#include "WaterAnimal.h"
#include "../../level/Level.h"


WaterAnimal::WaterAnimal( Level* level )
:   super(level)
{
}

bool WaterAnimal::isWaterMob()
{
	// prevent drowning
	return true;
}

void WaterAnimal::addAdditonalSaveData( CompoundTag* entityTag )
{
	super::addAdditonalSaveData(entityTag);
}

void WaterAnimal::readAdditionalSaveData( CompoundTag* tag )
{
	super::readAdditionalSaveData(tag);
}

bool WaterAnimal::canSpawn()
{
	return level->isUnobstructed(bb);
}

int WaterAnimal::getAmbientSoundInterval()
{
	return SharedConstants::TicksPerSecond * 6;
}

bool WaterAnimal::removeWhenFarAway()
{
	return true;
}

int WaterAnimal::getExperienceReward( Player* killedBy )
{
	return 1 + level->random.nextInt(3);
}

int WaterAnimal::getCreatureBaseType() const {
	return MobTypes::BaseWaterCreature;
}
