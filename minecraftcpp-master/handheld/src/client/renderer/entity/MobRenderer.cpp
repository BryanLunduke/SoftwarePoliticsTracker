#include "MobRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../../gui/Font.h"
#include "../Tesselator.h"
#include "../../Minecraft.h"
#include "../../model/Model.h"
#include "../../../world/entity/Mob.h"
#include "../../../util/Mth.h"

MobRenderer::MobRenderer(Model* model, float shadow)
:	model(model),
	armor(NULL)
{
	shadowRadius = shadow;
}

MobRenderer::~MobRenderer() {
	delete model;
}

void MobRenderer::setArmor(Model* armor) {
	this->armor = armor;
}

Model* MobRenderer::getArmor() {
	return armor;
}

void MobRenderer::render(Entity* e, float x, float y, float z, float rot, float a)
{
	Mob* mob = (Mob*) e;

	glPushMatrix2();
	glDisable2(GL_CULL_FACE);

	model->attackTime = getAttackAnim(mob, a);
	model->riding = false;//mob.isRiding();
	model->young = mob->isBaby();

	if (armor) {
		armor->riding = model->riding;
		armor->young  = model->young;
		armor->attackTime = model->attackTime;
	}

	float bodyRot = (mob->yBodyRotO + (mob->yBodyRot - mob->yBodyRotO) * a);
	float headRot = (mob->yRotO + (mob->yRot - mob->yRotO) * a);
	float headRotx = (mob->xRotO + (mob->xRot - mob->xRotO) * a);

	float yoffset = mob->heightOffset;
	setupPosition(mob, x, y - yoffset, z);

	float bob = getBob(mob, a);
	setupRotations(mob, bob, bodyRot, a);

	float ascale = 1 / 16.0f;
	glScalef2(-1, -1, 1);

	scale(mob, a);
	glTranslatef2(0, -24 * ascale - 0.125f / 16.0f, 0);

	float ws = mob->walkAnimSpeedO + (mob->walkAnimSpeed - mob->walkAnimSpeedO) * a;
	float wp = mob->walkAnimPos - mob->walkAnimSpeed * (1 - a);
	if (mob->isBaby()) {
		wp *= 3.0f;
	}

	if (ws > 1) ws = 1;

	bindTexture(mob->getTexture());
	//glEnable2(GL_ALPHA_TEST);

	model->prepareMobModel(mob, wp, ws, a);
	model->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);

	for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
		if (prepareArmor(mob, i, a) < 0) continue;

		armor->prepareMobModel(mob, wp, ws, a);
		armor->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);
		glDisable2(GL_BLEND);
		glEnable2(GL_ALPHA_TEST);
	}

	additionalRendering(mob, a);
	float br = mob->getBrightness(a);
	int overlayColor = getOverlayColor(mob, br, a);

	bool renderOverlay = ((overlayColor >> 24) & 0xff) > 0;
	bool renderHurt = (mob->hurtTime > 0) || (mob->deathTime > 0);

	if (renderOverlay || renderHurt) {
		glDisable2(GL_TEXTURE_2D);
		glDisable2(GL_ALPHA_TEST);
		glEnable2(GL_BLEND);
		glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthFunc(GL_EQUAL);

		if (renderHurt) {
			glEnable(GL_COLOR_MATERIAL);
			glColor4f2(br, 0, 0, 0.4f);
			model->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);
			for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
				if (prepareArmor(mob, i, a) < 0) continue;

				glColor4f2(br, 0, 0, 0.4f);
				armor->prepareMobModel(mob, wp, ws, a);
				armor->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);
			}
		}

		if (renderOverlay) {
			float r = ((overlayColor >> 16) & 0xff) / 255.0f;
			float g = ((overlayColor >> 8) & 0xff) / 255.0f;
			float b = ((overlayColor) & 0xff) / 255.0f;
			float aa = ((overlayColor >> 24) & 0xff) / 255.0f;
			glColor4f2(r, g, b, aa);
			model->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);
			for (int i = 0; i < MAX_ARMOR_LAYERS; i++) {
				if (prepareArmor(mob, i, a) < 0) continue;

				glColor4f2(r, g, b, aa);
				armor->prepareMobModel(mob, wp, ws, a);
				armor->render(e, wp, ws, bob, headRot - bodyRot, headRotx, ascale);
			}
		}

		glDepthFunc(GL_LEQUAL);
		glDisable2(GL_BLEND);
		glEnable2(GL_ALPHA_TEST);
		glEnable2(GL_TEXTURE_2D);
	}

	glEnable2(GL_CULL_FACE);
	//glEnable2(GL_DEPTH_TEST);

	glPopMatrix2();

	renderName(mob, x, y, z);
}

void MobRenderer::setupPosition(Entity* mob, float x, float y, float z) {
	glTranslatef2((float) x, (float) y, (float) z);
}

void MobRenderer::setupRotations(Entity* mob_, float bob, float bodyRot, float a) {
	glRotatef2(180 - bodyRot, 0, 1, 0);

	Mob* mob = (Mob*)mob_;
	if (mob->deathTime > 0) {
		float fall = (mob->deathTime + a - 1) / 20.0f * 1.6f;
		fall = Mth::sqrt(fall);
		if (fall > 1) fall = 1;
		glRotatef2(fall * getFlipDegrees(mob), 0, 0, 1);
	}
}

float MobRenderer::getAttackAnim(Mob* mob, float a) {
	return mob->getAttackAnim(a);
}

float MobRenderer::getBob(Mob* mob, float a) {
	return (mob->tickCount + a);
}

void MobRenderer::additionalRendering(Mob* mob, float a) {
}

void MobRenderer::onGraphicsReset() {
	if (model) model->onGraphicsReset();
	if (armor) armor->onGraphicsReset();
}

int MobRenderer::prepareArmor(Mob* mob, int layer, float a) {
	return -1;
}

float MobRenderer::getFlipDegrees(Mob* mob) {
	return 90;
}

int MobRenderer::getOverlayColor(Mob* mob, float br, float a) {
	return 0;
}

void MobRenderer::scale(Mob* mob, float a) {
}

void MobRenderer::renderName(Mob* mob, float x, float y, float z) {
	/*
	std::stringstream ss; ss << mob->entityId;
	renderNameTag(mob, ss.str(), x, y, z, 64);
	*/
}

void MobRenderer::renderNameTag(Mob* mob, const std::string& name, float x, float y, float z, int maxDist) {
	float dist = mob->distanceToSqr(entityRenderDispatcher->cameraEntity);

	if (dist > maxDist*maxDist)
		return;

	Font* font = getFont();

	float size = 1.60f;
	float s = 1 / 60.0f * size;

	glPushMatrix2();
	glTranslatef2((float) x + 0, (float) y + 1.0f /*2.3f*/, (float) z);

	glRotatef2(-entityRenderDispatcher->playerRotY, 0, 1, 0);
	glRotatef2(entityRenderDispatcher->playerRotX, 1, 0, 0);

	glScalef2(-s, -s, s);

	glDepthMask(false);
	glDisable2(GL_DEPTH_TEST);
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Tesselator& t = Tesselator::instance;

	glDisable2(GL_TEXTURE_2D);
	t.begin();
	int w = font->width(name) / 2;
	t.color(0.0f, 0.0f, 0.0f, 0.25f);
	t.vertex(-(float)w - 1, -1, 0);
	t.vertex(-(float)w - 1, +8, 0);
	t.vertex(+(float)w + 1, +8, 0);
	t.vertex(+(float)w + 1, -1, 0);
	//t.end();
	t.draw();
	glEnable2(GL_TEXTURE_2D);
	const float fnameWidth = (float)font->width(name) / -2;
	font->draw(name, fnameWidth, 0, 0x20ffffff);
	glEnable2(GL_DEPTH_TEST);

	glDepthMask(true);
	font->draw(name, (float) fnameWidth, 0, 0xffffffff);
	glDisable2(GL_BLEND);
	glColor4f2(1, 1, 1, 1);
	glPopMatrix2();
}
