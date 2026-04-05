#include "Skeleton.h"
#include "../projectile/Arrow.h"

Skeleton::Skeleton( Level* level )
:   super(level),
	bow(Item::bow, 1),
	fireCheckTick(0)
{
	entityRendererId = ER_SKELETON_RENDERER;
	this->textureName = "mob/skeleton.png";
}

int Skeleton::getMaxHealth() {
	return 10; // 15
}

void Skeleton::aiStep() {
	if ((++fireCheckTick & 1) && level->isDay() && !level->isClientSide) {
		float br = getBrightness(1);
		if (br > 0.5f) {
			if (level->canSeeSky(Mth::floor(x), Mth::floor(y), Mth::floor(z)) && random.nextFloat() * 3.5f < (br - 0.4f)) {
				hurt(NULL, 1);

				for (int i = 0; i < 5; ++i) {
					float xa = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
					float ya = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
					float za = (2.0f * random.nextFloat() - 1.0f) * (2.0f * random.nextFloat() - 1.0f) * 0.02f;
					level->addParticle(PARTICLETYPE(explode), x + random.nextFloat() * bbWidth * 2 - bbWidth, y + random.nextFloat() * bbHeight, z + random.nextFloat() * bbWidth * 2 - bbWidth, xa, ya, za);
				}
				//setOnFire(8); //@todo
			}
		}
	}

	super::aiStep();
}

int Skeleton::getDeathLoot() {
	return Item::arrow->id;
}

ItemInstance* Skeleton::getCarriedItem() {
	return &bow;
}

int Skeleton::getEntityTypeId() const {
	return MobTypes::Skeleton;
}

const char* Skeleton::getAmbientSound() {
	return "mob.skeleton";
}

std::string Skeleton::getHurtSound() {
	return "mob.skeletonhurt";
}

std::string Skeleton::getDeathSound() {
	return "mob.skeletonhurt";
}

void Skeleton::checkHurtTarget( Entity* target, float d ) {
	if (d < 10) {
		float xd = target->x - x;
		float zd = target->z - z;

		if (attackTime == 0) {
			Arrow* arrow = new Arrow(level, this, 1);
			//                arrow.y += 1.4f;

			float yd = (target->y + target->getHeadHeight() - 0.7f) - arrow->y;

			float yo = Mth::sqrt(xd * xd + zd * zd) * 0.2f;

			level->playSound(this, "random.bow", 1.0f, 1 / (random.nextFloat() * 0.4f + 0.8f));
			level->addEntity(arrow);

			arrow->shoot(xd, yd + yo, zd, 1.60f, 32);
			attackTime = SharedConstants::TicksPerSecond * 3;
		}
		yRot = (float) (std::atan2(zd, xd) * Mth::RADDEG) - 90;

		holdGround = true;
	}
}

void Skeleton::dropDeathLoot( /*bool wasKilledByPlayer, int playerBonusLevel*/ ) {
	// drop some arrows
	int count = random.nextInt(3 /*+ playerBonusLevel*/);
	for (int i = 0; i < count; i++) {
		spawnAtLocation(Item::arrow->id, 1);
	}
	// and some bones
	count = random.nextInt(3 /*+ playerBonusLevel*/);
	for (int i = 0; i < count; i++) {
		spawnAtLocation(Item::bone->id, 1);
	}
}

int Skeleton::getUseDuration() {
	return attackTime;
}

/*@Override*/
//     MobType getMobType() {
//         return MobType.UNDEAD;
//     }
