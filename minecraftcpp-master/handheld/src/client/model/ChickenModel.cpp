#include "ChickenModel.h"
#include "../../world/entity/Entity.h"

ChickenModel::ChickenModel()
:   head(0, 0),
	beak(14, 0),
	redThing(14, 4),
	body(0, 9),
	leg0(26, 0),
	leg1(26, 0),
	wing0(24, 13),
	wing1(24, 13)
{
	head.setModel(this);
	beak.setModel(this);
	redThing.setModel(this);
	body.setModel(this);
	leg0.setModel(this);
	leg1.setModel(this);
	wing0.setModel(this);
	wing1.setModel(this);

	int yo = 16;
	head.addBox(-2, -6, -2, 4, 6, 3, 0); // Head
	head.setPos(0, -1 + (float)yo, -4);

	beak.addBox(-2, -4, -4, 4, 2, 2, 0); // Beak
	beak.setPos(0, -1 + (float)yo, -4);

	redThing.addBox(-1, -2, -3, 2, 2, 2, 0); // Beak
	redThing.setPos(0, -1 + (float)yo, -4);

	body.addBox(-3, -4, -3, 6, 8, 6, 0); // Body
	body.setPos(0, 0 + (float)yo, 0);

	leg0.addBox(-1, 0, -3, 3, 5, 3); // Leg0
	leg0.setPos(-2, 3 + (float)yo, 1);

	leg1.addBox(-1, 0, -3, 3, 5, 3); // Leg1
	leg1.setPos(1, 3 + (float)yo, 1);

	wing0.addBox(0, 0, -3, 1, 4, 6); // Wing0
	wing0.setPos(-4, -3 + (float)yo, 0);

	wing1.addBox(-1, 0, -3, 1, 4, 6); // Wing1
	wing1.setPos(4, -3 + (float)yo, 0);
}

void ChickenModel::render( Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale )
{
	setupAnim(time, r, bob, yRot, xRot, scale);

	if (young) {
		float ss = 2;
		glPushMatrix();
		glTranslatef(0, 5 * scale, 2 * scale);
		head.render(scale);
		beak.render(scale);
		redThing.render(scale);
		glPopMatrix();
		glPushMatrix();
		glScalef(1 / ss, 1 / ss, 1 / ss);
		glTranslatef(0, 24 * scale, 0);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		wing0.render(scale);
		wing1.render(scale);
		glPopMatrix();
	} else {
		head.render(scale);
		beak.render(scale);
		redThing.render(scale);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		wing0.render(scale);
		wing1.render(scale);
	}
}

void ChickenModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	head.xRot = -((xRot / (float) (180 / Mth::PI)));
	head.yRot = yRot / (float) (180 / Mth::PI);

	beak.xRot = head.xRot;
	beak.yRot = head.yRot;

	redThing.xRot = head.xRot;
	redThing.yRot = head.yRot;

	body.xRot = 90 / (float) (180 / Mth::PI);

	const float pend = Mth::cos(time * 0.6662f) * 1.4f * r;
	leg0.xRot =  pend;
	leg1.xRot = -pend;
	wing0.zRot = bob;
	wing1.zRot = -bob;
}

