#include "SheepModel.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Mob.h"
#include "../../world/entity/animal/Sheep.h"

SheepModel::SheepModel()
:   super(12, 0)
{
	head = ModelPart(this, 0, 0);
	head.addBox(-3, -4, -6, 6, 6, 8, 0); // Head
	head.setPos(0, 12 - 6, -8);

	body = ModelPart(this, 28, 8);
	body.addBox(-4, -10, -7, 8, 16, 6, 0); // Body
	body.setPos(0, 11 + 6 - 12, 2);
}

void SheepModel::prepareMobModel( Mob* mob, float time, float r, float a )
{
	super::prepareMobModel(mob, time, r, a);

	Sheep* sheep = (Sheep*) mob;
	head.y = 6 + sheep->getHeadEatPositionScale(a) * 9.f;
	headXRot = sheep->getHeadEatAngleScale(a);
}

void SheepModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	super::setupAnim(time, r, bob, yRot, xRot, scale);

	head.xRot = headXRot;
}
