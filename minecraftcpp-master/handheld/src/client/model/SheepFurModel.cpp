#include "SheepFurModel.h"
#include "geom/ModelPart.h"
#include "../../world/entity/Mob.h"
#include "../../world/entity/animal/Sheep.h"

SheepFurModel::SheepFurModel()
:   super(12, 0)
{
	head = ModelPart(this, 0, 0);
	head.addBox(-3, -4, -4, 6, 6, 6, 0.6f); // Head
	head.setPos(0, 12 - 6, -8);

	body = ModelPart(this, 28, 8);
	body.addBox(-4, -10, -7, 8, 16, 6, 1.75f); // Body
	body.setPos(0, 11 + 6 - 12, 2);

	float g = 0.5f;
	leg0 = ModelPart(this, 0, 16);
	leg0.addBox(-2, 0, -2, 4, 6, 4, g); // Leg0
	leg0.setPos(-3, 18 + 6 - 12, 7);

	leg1 = ModelPart(this, 0, 16);
	leg1.addBox(-2, 0, -2, 4, 6, 4, g); // Leg1
	leg1.setPos(3, 18 + 6 - 12, 7);

	leg2 = ModelPart(this, 0, 16);
	leg2.addBox(-2, 0, -2, 4, 6, 4, g); // Leg2
	leg2.setPos(-3, 18 + 6 - 12, -5);

	leg3 = ModelPart(this, 0, 16);
	leg3.addBox(-2, 0, -2, 4, 6, 4, g); // Leg3
	leg3.setPos(3, 18 + 6 - 12, -5);
}

void SheepFurModel::prepareMobModel( Mob* mob, float time, float r, float a )
{
	super::prepareMobModel(mob, time, r, a);

	Sheep* sheep = (Sheep*) mob;
	head.y = 6 + sheep->getHeadEatPositionScale(a) * 9.f;
	headXRot = sheep->getHeadEatAngleScale(a);
}

void SheepFurModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	super::setupAnim(time, r, bob, yRot, xRot, scale);

	head.xRot = headXRot;
}
