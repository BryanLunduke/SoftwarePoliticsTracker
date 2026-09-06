#include "TripodCameraRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../Tesselator.h"
#include "../../Minecraft.h"
#include "../../../world/entity/item/TripodCamera.h"
#include "../../../world/level/material/Material.h"

TripodCameraRenderer::TripodCameraRenderer()
:	tripod(0, 15 * 16 + 3, Material::plant),
	cameraCube(0, 0)
{
	cameraCube.addBox(-4, -4, -6, 8, 8, 10);
	cameraCube.y = 11;
	this->shadowRadius = 0.5f;
}

float TripodCameraRenderer::getFlashTime(const TripodCamera* c, float a) {
	if (c->life >= 8) return -1;
	if (c->life < 0) return -1;
	return (c->life - a) * 0.125f;
}

void TripodCameraRenderer::render(Entity* cam_, float x, float y, float z, float rot, float a) {
	TripodCamera* cam = (TripodCamera*)cam_;

	glPushMatrix2();
	glTranslatef2((float) x, (float) y, (float) z);

	cameraCube.xRot = Mth::DEGRAD * (180.0f + cam->xRot * 0.5f);
	cameraCube.yRot = -Mth::DEGRAD * cam->yRot;

	//float br = (1 - ((cam->life - a + 1) / 100.0f)) * 0.8f;

	Tesselator& t = Tesselator::instance;
	t.color(1.0f, 1.0f, 1.0f);

	//printf("xyz: %f, %f, %f\n", x, y, z);

	// Render tripod legs
	bindTexture("gui/items.png");
	t.begin();
	tileRenderer.tesselateCrossTexture(&tripod, 0, -0.5f, -0.5f, -0.5f);//, y, z);
	t.draw();

	// Render tripod
	bindTexture("item/camera.png");
	cameraCube.render(1.0f / 16.0f);

	bool isCurrentlyPicked = entityRenderDispatcher->minecraft->hitResult.entity == cam;

	const float flashLife = getFlashTime(cam, a);

	if (flashLife >= 0) {
		const float flashStrength = ::sin(flashLife * 6.2830f);

		// Flash
		glColor4f2(1, 1, 1, flashStrength);
		glColor4f2(1, 1, 1, 1);
	}

	// "red light" flashing when photo is about to go off
	if (isCurrentlyPicked) {
		glDisable2(GL_TEXTURE_2D);
		glEnable2(GL_BLEND);
		glBlendFunc2(GL_SRC_ALPHA, GL_DST_ALPHA);
		glColor4f2(0.5f,0.5f,0.5f,0.5f);

		cameraCube.renderHorrible(1.0f / 16.0f);

		glColor4f2(1,1,1,1);
		glDisable2(GL_BLEND);
		glEnable2(GL_TEXTURE_2D);
	}
	glPopMatrix2();
}
