#include "Chicken.h"
#include "../../item/Item.h"
#include "../../level/Level.h"

Chicken::Chicken( Level* level )
:   super(level),
	sheared(false),
	flap(0),
	oFlap(0),
	flapSpeed(0),
	oFlapSpeed(0),
	flapping(1),
	eggTime(0)
{
	entityRendererId = ER_CHICKEN_RENDERER;

	textureName = "mob/chicken.png";
	setSize(0.3f, 0.7f);
	eggTime = random.nextInt(SharedConstants::TicksPerSecond * 60 * 5) + SharedConstants::TicksPerSecond * 60 * 5;
}

int Chicken::getEntityTypeId() const
{
	return MobTypes::Chicken;
}


int Chicken::getMaxHealth()
{
	return 4;
}

void Chicken::aiStep()
{
	super::aiStep();

	oFlap = flap;
	oFlapSpeed = flapSpeed;

	flapSpeed += (onGround ? -1 : 4) * 0.3f;
	if (flapSpeed < 0) flapSpeed = 0;
	if (flapSpeed > 1) flapSpeed = 1;

	if (!onGround && flapping < 1) flapping = 1;
	flapping *= 0.9f;

	if (!onGround && yd < 0) {
		yd *= 0.6f;
	}

	flap += flapping * 2;

	//@todo
	//if (!isBaby()) {
	//	if (!level->isClientSide && --eggTime <= 0) {
	//		level->playSound(this, "mob.chickenplop", 1.0f, (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
	//		spawnAtLocation(Item::egg->id, 1);
	//		eggTime = random.nextInt(SharedConstants::TicksPerSecond * 60 * 5) + SharedConstants::TicksPerSecond * 60 * 5;
	//	}
	//}
}

void Chicken::addAdditonalSaveData( CompoundTag* tag )
{
	super::addAdditonalSaveData(tag);
}

void Chicken::readAdditionalSaveData( CompoundTag* tag )
{
	super::readAdditionalSaveData(tag);
}

void Chicken::causeFallDamage( float distance )
{
}

const char* Chicken::getAmbientSound()
{
	return "mob.chicken";
}

std::string Chicken::getHurtSound()
{
	return "mob.chickenhurt";
}

std::string Chicken::getDeathSound()
{	
	return "mob.chickenhurt";
}

//int Chicken::getDeathLoot()
//{
//	return Item::feather->id;
//}

void Chicken::dropDeathLoot( /*bool wasKilledByPlayer, int playerBonusLevel*/ )
{
	//// drop some feathers
	int count = random.nextInt(3);// + random.nextInt(1 + playerBonusLevel);
	for (int i = 0; i < count; i++) {
		spawnAtLocation(Item::feather->id, 1);
	}
	//// and some meat
	//if (isOnFire()) spawnAtLocation(Item::chicken_cooked->id, 1); //@fire
	//else
	    spawnAtLocation(Item::chicken_raw->id, 1);
}

Animal* Chicken::getBreedOffspring( Animal* target )
{
	return new Chicken(level);
}
