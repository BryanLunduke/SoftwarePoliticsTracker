#include "PrimedTnt.h"
#include "../../../nbt/CompoundTag.h"

PrimedTnt::PrimedTnt( Level* level )
:	super(level),
	life(80)
{
	entityRendererId = ER_TNT_RENDERER ;

	blocksBuilding = true;
	setSize(0.98f, 0.98f);
	heightOffset = bbHeight / 2.0f;
}

PrimedTnt::PrimedTnt( Level* level, float x, float y, float z )
:	super(level),
	life(80)
{
	entityRendererId = ER_TNT_RENDERER ;

	blocksBuilding = true;
	setSize(0.98f, 0.98f);
	heightOffset = bbHeight / 2.0f;

	setPos(x, y, z);

	float rot = Mth::random() * Mth::PI * 2.0f;
	xd = Mth::sin(rot * Mth::DEGRAD) * -0.02f;
	yd = +0.2f;
	zd = Mth::cos(rot * Mth::DEGRAD) * -0.02f;

	makeStepSound = false;

	xo = x;
	yo = y;
	zo = z;
}

bool PrimedTnt::isPickable()
{
	return !removed;
}

void PrimedTnt::tick()
{
	xo = x;
	yo = y;
	zo = z;

	yd -= 0.04f;
	move(xd, yd, zd);
	xd *= 0.98f;
	yd *= 0.98f;
	zd *= 0.98f;

	if (onGround) {
		xd *= 0.7f;
		zd *= 0.7f;
		yd *= -0.5f;
	}
	life--;
	if (!level->isClientSide && life <= 0) {
		remove();
		explode();
	} else {
		level->addParticle(PARTICLETYPE(smoke), x, y + 0.5f, z, 0, 0, 0);
	}
}

float PrimedTnt::getShadowHeightOffs()
{
	return 0;
}

void PrimedTnt::explode()
{
	float r = 3.1f;
	level->explode(NULL, x, y, z, r);
}

void PrimedTnt::addAdditonalSaveData(CompoundTag* entityTag) {
    entityTag->putByte("Fuse", life);
}

void PrimedTnt::readAdditionalSaveData(CompoundTag* tag) {
    life = tag->getByte("Fuse");
}

int PrimedTnt::getEntityTypeId() const {
	return EntityTypes::IdPrimedTnt;
}
