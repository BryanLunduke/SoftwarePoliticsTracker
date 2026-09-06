#include "AgableMob.h"

AgableMob::AgableMob( Level* level )
:   super(level),
	age(-1)
{
	entityData.define(DATA_FLAGS_ID, (SynchedEntityData::TypeChar)0);
	setAge(0);
}

int AgableMob::getAge() {
	return age;
}

void AgableMob::setAge( int age ) {
	if (this->age < 0 && age >= 0) {
		entityData.clearFlag<SynchedEntityData::TypeChar>(DATA_FLAGS_ID, DATAFLAG_ISBABY);
	}
	else if (this->age >= 0 && age < 0) {
		entityData.setFlag<SynchedEntityData::TypeChar>(DATA_FLAGS_ID, DATAFLAG_ISBABY);
	}
	this->age = age;
}

void AgableMob::addAdditonalSaveData( CompoundTag* tag ) {
    super::addAdditonalSaveData(tag);
    tag->putInt("Age", getAge());
}

void AgableMob::readAdditionalSaveData( CompoundTag* tag ) {
    super::readAdditionalSaveData(tag);
    setAge(tag->getInt("Age"));
}

void AgableMob::aiStep() {
	super::aiStep();

	//@note: keeping this for now, since we don't use breeding anyway
	//       and it feels better to have animals at age 0 then 99999999
	//       if we decide to actually use it.
	if (age < 0)
		setAge(age + 1);
	else if (age > 0)
		setAge(age - 1);
}

bool AgableMob::isBaby() {
	if (!level->isClientSide) {
		return age < 0;
	} else {
		return entityData.getFlag<SynchedEntityData::TypeChar>(DATA_FLAGS_ID, DATAFLAG_ISBABY);
	}
}
