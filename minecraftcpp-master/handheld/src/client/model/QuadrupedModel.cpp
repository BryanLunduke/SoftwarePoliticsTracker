#include "QuadrupedModel.h"
#include "geom/ModelPart.h"
#include "../../util/Mth.h"

QuadrupedModel::QuadrupedModel( int legSize, float g )
:	yHeadOffs(8),
	zHeadOffs(4),
	head(0, 0),
	body(28, 8),
	leg0(0, 16),
	leg1(0, 16),
	leg2(0, 16),
	leg3(0, 16)
{
	head.setModel(this);
	body.setModel(this);
	leg0.setModel(this);
	leg1.setModel(this);
	leg2.setModel(this);
	leg3.setModel(this);

	head.addBox(-4, -4, -8, 8, 8, 8, g); // Head
	head.setPos(0, 12 + 6 - (float)legSize, -6);

	body.addBox(-5, -10, -7, 10, 16, 8, g); // Body
	body.setPos(0, 11 + 6 - (float)legSize, 2);

	leg0.addBox(-2, 0, -2, 4, legSize, 4, g); // Leg0
	leg0.setPos(-3, 18 + 6 - (float)legSize, 7);

	leg1.addBox(-2, 0, -2, 4, legSize, 4, g); // Leg1
	leg1.setPos(3, 18 + 6 - (float)legSize, 7);

	leg2.addBox(-2, 0, -2, 4, legSize, 4, g); // Leg2
	leg2.setPos(-3, 18 + 6 - (float)legSize, -5);

	leg3.addBox(-2, 0, -2, 4, legSize, 4, g); // Leg3
	leg3.setPos(3, 18 + 6 - (float)legSize, -5);
}

void QuadrupedModel::render( Entity* entity, float time, float r, float bob, float yRot, float xRot, float scale )
{
	setupAnim(time, r, bob, yRot, xRot, scale);

	if (young) {
		float ss = 2.0f;
		glPushMatrix();
		glTranslatef(0, yHeadOffs * scale, zHeadOffs * scale);
		head.render(scale);
		glPopMatrix();
		glPushMatrix();
		glScalef(1 / ss, 1 / ss, 1 / ss);
		glTranslatef(0, 24 * scale, 0);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		leg2.render(scale);
		leg3.render(scale);
		glPopMatrix();
	} else {
		head.render(scale);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		leg2.render(scale);
		leg3.render(scale);
	}
}

void QuadrupedModel::render( QuadrupedModel* model, float scale )
{
	head.yRot = model->head.yRot;
	head.xRot = model->head.xRot;

	head.y = model->head.y;
	head.x = model->head.x;

	body.yRot = model->body.yRot;
	body.xRot = model->body.xRot;

	leg0.xRot = model->leg0.xRot;
	leg1.xRot = model->leg1.xRot;
	leg2.xRot = model->leg2.xRot;
	leg3.xRot = model->leg3.xRot;

	if (young) {
		float ss = 2.0f;
		glPushMatrix();
		glTranslatef(0, 8 * scale, 4 * scale);
		head.render(scale);
		glPopMatrix();
		glPushMatrix();
		glScalef(1 / ss, 1 / ss, 1 / ss);
		glTranslatef(0, 24 * scale, 0);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		leg2.render(scale);
		leg3.render(scale);
		glPopMatrix();
	} else {
		head.render(scale);
		body.render(scale);
		leg0.render(scale);
		leg1.render(scale);
		leg2.render(scale);
		leg3.render(scale);
	}
}

void QuadrupedModel::setupAnim( float time, float r, float bob, float yRot, float xRot, float scale )
{
	head.xRot = xRot / (180.f / Mth::PI);
	head.yRot = yRot / (180.f / Mth::PI);
	body.xRot = 90 / (180.f / Mth::PI);

	const float pend = Mth::cos(time * 0.6662f) * 1.4f * r;
	leg0.xRot =  pend;
	leg1.xRot = -pend;
	leg2.xRot = -pend;
	leg3.xRot =  pend;
}
