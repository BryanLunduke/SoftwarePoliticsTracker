#include "EntityRenderer.h"

#include "../Tesselator.h"
#include "../Textures.h"
#include "../gles.h"
#include "../../../world/phys/AABB.h"
#include "EntityRenderDispatcher.h"

EntityRenderDispatcher* EntityRenderer::entityRenderDispatcher = NULL;

EntityRenderer::EntityRenderer()
:	shadowRadius(0),
	shadowStrength(1.0f)
{}

void EntityRenderer::bindTexture(const std::string& resourceName) {
	entityRenderDispatcher->textures->loadAndBindTexture(resourceName);
}

//bool bindTexture(std::string urlTexture, std::string backupTexture) {
//    Textures t = entityRenderDispatcher.textures;

//    int id = t.loadHttpTexture(urlTexture, backupTexture);
//    if (id >= 0) {
//        t.bind(id);
//        return true;
//    } else {
//        return false;
//    }
//}

/*static*/
void EntityRenderer::render(const AABB& bb, float xo, float yo, float zo) {
	glDisable2(GL_TEXTURE_2D);
	Tesselator& t = Tesselator::instance;
	glColor4f2(1, 1, 1, 1);
	t.begin();
	t.offset(xo, yo, zo);
	//t.normal(0, 0, -1);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);

	//t.normal(0, 0, 1);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);

	//t.normal(0, -1, 0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z1);

	//t.normal(0, 1, 0);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y1, bb.z0);

	//t.normal(-1, 0, 0);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);

	//t.normal(1, 0, 0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.offset(0, 0, 0);
	t.draw();
	glEnable2(GL_TEXTURE_2D);
	// model.render(0, 1)
}

/*static*/
void EntityRenderer::renderFlat(const AABB& bb) {
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z1);
	t.vertex(bb.x0, bb.y1, bb.z0);
	t.vertex(bb.x0, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y0, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z0);
	t.vertex(bb.x1, bb.y1, bb.z1);
	t.vertex(bb.x1, bb.y0, bb.z1);
	t.draw();
}

void EntityRenderer::init(EntityRenderDispatcher* entityRenderDispatcher) {
	this->entityRenderDispatcher = entityRenderDispatcher;
}

Font* EntityRenderer::getFont() {
	return entityRenderDispatcher->getFont();
}

//void postRender(Entity entity, float x, float y, float z, float rot, float a) {
//    if (entityRenderDispatcher.options.fancyGraphics && shadowRadius > 0) {
//        float dist = entityRenderDispatcher.distanceToSqr(entity.x, entity.y, entity.z);
//        float pow = (float) ((1 - dist / (16.0f * 16.0f)) * shadowStrength);
//        if (pow > 0) {
//            renderShadow(entity, x, y, z, pow, a);
//        }
//    }
//    if (entity.isOnFire()) renderFlame(entity, x, y, z, a);
//}

//void renderFlame(Entity e, float x, float y, float z, float a) {
//	glDisable2(GL_LIGHTING);
//	int tex = Tile.fire.tex;

//	int xt = (tex & 0xf) << 4;
//	int yt = tex & 0xf0;

//	float u0 = (xt) / 256.0f;
//	float u1 = (xt + 15.99f) / 256.0f;
//	float v0 = (yt) / 256.0f;
//	float v1 = (yt + 15.99f) / 256.0f;

//	glPushMatrix2();
//	glTranslatef2((float) x, (float) y, (float) z);

//	float s = e.bbWidth * 1.4f;
//	glScalef2(s, s, s);
//	bindTexture("terrain.png");
//	Tesselator t = Tesselator.instance;

//	float r = 1.0f;
//	float xo = 0.5f;
//	float yo = 0.0f;

//	float h = e.bbHeight / e.bbWidth;

//	glRotatef2(-entityRenderDispatcher.playerRotY, 0, 1, 0);
//	glTranslatef2(0, 0, -0.4f + ((int) h) * 0.02f);
//	glColor4f2(1, 1, 1, 1);
//	// glRotatef2(-playerRotX, 1, 0, 0);
//	t.begin();
//	while (h > 0) {
//		t.vertexUV(r - xo, 0 - yo, 0, u1, v1);
//		t.vertexUV(0 - xo, 0 - yo, 0, u0, v1);
//		t.vertexUV(0 - xo, 1.4f - yo, 0, u0, v0);
//		t.vertexUV(r - xo, 1.4f - yo, 0, u1, v0);
//		h -= 1;
//		yo -= 1;
//		r *= 0.9f;
//		glTranslatef2(0, 0, -0.04f);
//	}
//	t.end();
//	glPopMatrix2();
//	glEnable2(GL_LIGHTING);
//}

//void renderShadow(Entity e, float x, float y, float z, float pow, float a) {
//	glEnable2(GL_BLEND);
//	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

//	Textures textures = entityRenderDispatcher.textures;
//	textures.bind(textures.loadTexture("%clamp%/misc/shadow.png"));

//	Level level = getLevel();

//	glDepthMask(false);
//	float r = shadowRadius;

//	float ex = e.xOld + (e.x - e.xOld) * a;
//	float ey = e.yOld + (e.y - e.yOld) * a + e.getShadowHeightOffs();
//	float ez = e.zOld + (e.z - e.zOld) * a;

//	int x0 = Mth.floor(ex - r);
//	int x1 = Mth.floor(ex + r);
//	int y0 = Mth.floor(ey - r);
//	int y1 = Mth.floor(ey);
//	int z0 = Mth.floor(ez - r);
//	int z1 = Mth.floor(ez + r);

//	float xo = x - ex;
//	float yo = y - ey;
//	float zo = z - ez;

//	Tesselator tt = Tesselator.instance;
//	tt.begin();
//	for (int xt = x0; xt <= x1; xt++)
//		for (int yt = y0; yt <= y1; yt++)
//			for (int zt = z0; zt <= z1; zt++) {
//				int t = level.getTile(xt, yt - 1, zt);
//				if (t > 0 && level.getRawBrightness(xt, yt, zt) > 3) {
//					renderTileShadow(Tile.tiles[t], x, y + e.getShadowHeightOffs(), z, xt, yt, zt, pow, r, xo, yo + e.getShadowHeightOffs(), zo);
//				}
//			}
//			tt.end();

//			glColor4f2(1, 1, 1, 1);
//			glDisable2(GL_BLEND);
//			glDepthMask(true);
//}

//Level* getLevel() {
//	return entityRenderDispatcher.level;
//}

//void renderTileShadow(Tile tt, float x, float y, float z, int xt, int yt, int zt, float pow, float r, float xo, float yo, float zo) {
//	Tesselator t = Tesselator.instance;
//	if (!tt.isCubeShaped()) return;

//	float a = ((pow - (y - (yt + yo)) / 2) * 0.5f) * getLevel().getBrightness(xt, yt, zt);
//	if (a < 0) return;
//	if (a > 1) a = 1;
//	t.color(1, 1, 1, (float) a);
//	// glColor4f2(1, 1, 1, (float) a);

//	float x0 = xt + tt.xx0 + xo;
//	float x1 = xt + tt.xx1 + xo;
//	float y0 = yt + tt.yy0 + yo + 1.0 / 64.0f;
//	float z0 = zt + tt.zz0 + zo;
//	float z1 = zt + tt.zz1 + zo;

//	float u0 = (float) ((x - (x0)) / 2 / r + 0.5f);
//	float u1 = (float) ((x - (x1)) / 2 / r + 0.5f);
//	float v0 = (float) ((z - (z0)) / 2 / r + 0.5f);
//	float v1 = (float) ((z - (z1)) / 2 / r + 0.5f);

//	t.vertexUV(x0, y0, z0, u0, v0);
//	t.vertexUV(x0, y0, z1, u0, v1);
//	t.vertexUV(x1, y0, z1, u1, v1);
//	t.vertexUV(x1, y0, z0, u1, v0);
//}
