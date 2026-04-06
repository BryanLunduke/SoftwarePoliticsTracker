#include "ArrowRenderer.h"

#include "../Tesselator.h"
#include "../Textures.h"
#include "../gles.h"
#include "../../../world/entity/Entity.h"
#include "../../../world/entity/projectile/Arrow.h"

void ArrowRenderer::render(Entity* entity, float x, float y, float z, float rot, float a) {
	bindTexture("item/arrows.png");
	Arrow* arrow = (Arrow*) entity;
	glPushMatrix2();
	glTranslatef(x, y, z);
	glRotatef(arrow->yRotO + (arrow->yRot - arrow->yRotO) * a - 90, 0, 1, 0);
	glRotatef(arrow->xRotO + (arrow->xRot - arrow->xRotO) * a, 0, 0, 1);
	Tesselator& t = Tesselator::instance;

	int type = 0;

	float u0 = 0 / 32.0f;
	float u1 = 16 / 32.0f;
	float v0 = (0 + type * 10) / 32.0f;
	float v1 = (5 + type * 10) / 32.0f;

	float u02 = 0 / 32.0f;
	float u12 = 5 / 32.0f;
	float v02 = (5 + type * 10) / 32.0f;
	float v12 = (10 + type * 10) / 32.0f;
	float ss = 0.9f / 16.0f;
	float shake = arrow->shakeTime - a;
	if (shake > 0) {
		float pow = -Mth::sin(shake * 3) * shake;
		glRotatef(pow, 0, 0, 1);
	}
	glRotatef(45, 1, 0, 0);
	glScalef(ss, ss, ss);

	glTranslatef(-4, 0, 0);

	t.begin();
	t.vertexUV(-7, -2, -2, u02, v02);
	t.vertexUV(-7, -2, +2, u12, v02);
	t.vertexUV(-7, +2, +2, u12, v12);
	t.vertexUV(-7, +2, -2, u02, v12);

	t.vertexUV(-7, +2, -2, u02, v02);
	t.vertexUV(-7, +2, +2, u12, v02);
	t.vertexUV(-7, -2, +2, u12, v12);
	t.vertexUV(-7, -2, -2, u02, v12);
	t.draw();

	for (int i = 0; i < 4; i++) {
		glRotatef(90, 1, 0, 0);
		t.begin();
		t.vertexUV(-8, -2, 0, u0, v0);
		t.vertexUV(+8, -2, 0, u1, v0);
		t.vertexUV(+8, +2, 0, u1, v1);
		t.vertexUV(-8, +2, 0, u0, v1);
		t.endOverrideAndDraw();
	}

	glPopMatrix2();
}