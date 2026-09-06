#include "ItemRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../Tesselator.h"
#include "../TileRenderer.h"
#include "../Textures.h"
#include "../../gui/Font.h"
#include "../../../world/entity/item/ItemEntity.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../util/Mth.h"
#include "../../../util/Random.h"
#include "EntityRenderer.h"
#include "../ItemInHandRenderer.h"
#include "../../gui/Gui.h"
#include "../../../world/item/Item.h"

/*static*/
TileRenderer* ItemRenderer::tileRenderer = new TileRenderer();

ItemRenderer::ItemRenderer()
{
	shadowRadius = 0.15f;
	shadowStrength = 0.75f;
}

void ItemRenderer::teardown_static() {
	if (tileRenderer) {
		delete tileRenderer;
		tileRenderer = NULL;
	}
}

void ItemRenderer::render(Entity* itemEntity_, float x, float y, float z, float rot, float a) {
	ItemEntity* itemEntity = (ItemEntity*) itemEntity_;
	random.setSeed(187);
	ItemInstance* item = &itemEntity->item;

	glPushMatrix2();
	float bob = Mth::sin((itemEntity->age + a) / 10.0f + itemEntity->bobOffs) * 0.1f + 0.1f;
	float spin = ((itemEntity->age + a) / 20.0f + itemEntity->bobOffs) * Mth::RADDEG;

	int count = 1;
	if (item->count > 20) count = 4;
	else if (item->count > 5) count = 3;
	else if (item->count > 1) count = 2;

	glTranslatef2((float) x, (float) y + bob, (float) z);
	//glEnable2(GL_RESCALE_NORMAL);
	if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
		glRotatef2(spin, 0, 1, 0);

		float br = itemEntity->getBrightness(a);
		if (item->id == Tile::sand->id || item->id == Tile::sandStone->id) br *= 0.8f;
		glColor4f2(br, br, br, 1.0f);

		bindTexture("terrain.png");
		float s = 1 / 4.0f;
		//if (!Tile::tiles[item->id]->isCubeShaped() && item->id != Tile::stoneSlabHalf->id) {
		const int shape = Tile::tiles[item->id]->getRenderShape();
		if (shape == Tile::SHAPE_CROSS_TEXTURE || shape == Tile::SHAPE_TORCH)
			s = 0.5f;

		glScalef2(s, s, s);
		for (int i = 0; i < count; i++) {
			if (i > 0) {
				glPushMatrix2();
				float xo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				float yo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				float zo = (random.nextFloat() * 2 - 1) * 0.2f / s;
				glTranslatef2(xo, yo, zo);
			}
			//static Stopwatch w;
			//w.start();
			entityRenderDispatcher->itemInHandRenderer->renderItem(NULL, item);
			//tileRenderer->renderTile(Tile::tiles[item->id], item->getAuxValue());
			//w.stop();
			//w.printEvery(100, "render-item");
			if (i > 0) glPopMatrix2();
		}
	} else {
		glScalef2(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
		int icon = item->getIcon();
		if (item->id < 256) {
			bindTexture("terrain.png");
		} else {
			bindTexture("gui/items.png");
		}
		Tesselator& t = Tesselator::instance;

		float u0 = ((icon % 16) * 16 + 0) / 256.0f;
		float u1 = ((icon % 16) * 16 + 16) / 256.0f;
		float v0 = ((icon / 16) * 16 + 0) / 256.0f;
		float v1 = ((icon / 16) * 16 + 16) / 256.0f;

		float r = 1.0f;
		float xo = 0.5f;
		float yo = 0.25f;

		// glRotatef2(-playerRotX, 1, 0, 0);
		for (int i = 0; i < count; i++) {
			glPushMatrix2();
			if (i > 0) {
				float _xo = (random.nextFloat() * 2 - 1) * 0.3f;
				float _yo = (random.nextFloat() * 2 - 1) * 0.3f;
				float _zo = (random.nextFloat() * 2 - 1) * 0.3f;
				glTranslatef2(_xo, _yo, _zo);
			}
			glRotatef2(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
			t.begin();
			//t.normal(0, 1, 0);
			t.vertexUV(0 - xo, 0 - yo, 0, u0, v1);
			t.vertexUV(r - xo, 0 - yo, 0, u1, v1);
			t.vertexUV(r - xo, 1 - yo, 0, u1, v0);
			t.vertexUV(0 - xo, 1 - yo, 0, u0, v0);
			//t.end();
			t.draw();

			glPopMatrix2();
		}
	}
	//glDisable2(GL_RESCALE_NORMAL);
	glPopMatrix2();
}


// @note: _18 -> a,b,c,-1,   a,b,c-1, ...
static const signed short _6[] = {139, 140, 141, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _17[] = {16, 17, 18, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _18[] = {79, 80, 81, -1, 79, 80, 81, -1, 79, 80, 81, -1, 79, 80, 81, -1};
static const signed short _24[] = {11, 12, 13, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _35[] = {52, 59, 58, 57, 56, 55, 54, 53, 67, 66, 65, 64, 63, 62, 61, 60};
static const signed short _44[] = {28, 32, 30, 29, 31, 33, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _98[] = {1, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _155[] = {34, 36, 35, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _263[] = {230, 151, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
static const signed short _351[] = {-1, 152, 154, -1, 193, 215, 216, -1, -1, 217, 218, 219, 220, 221, 222, 144};

static const signed short _mapper[] = {-1, 7, 9, 8, 0, 5, -2, -1, -1, -1, -1, -1, 14, 15, 39, 38, 37, -2, -2, -1, 49, 41, 46, -1, -2, -1, -1, -1, -1, -1, 235, -1, -1, -1, -1, -2, -1, 134, 135, 136, 137, 43, 44, -1, -2, 6, 76, 71, 4, 47, 129, -1, -1, 22, 74, -1, 40, 45, 72, -1, -1, 75, -1, -1, -1, 128, -1, 21, -1, -1, -1, -1, -1, 42, -1, -1, -1, -1, -1, -1, 48, 77, 10, 236, -1, 69, -1, 20, -1, 50, -1, -1, -1, -1, -1, -1, 68, -1, -2, -1, -1, -1, 130, 78, -1, -1, -1, 70, 23, 25, -1, -1, 19, -1, 26, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 24, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2, 27, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 73, -1, 51, -1, -1, -1, -1, -1, 82, -1, -1, 174, 173, 175, 231, 234, 147, 190, -2, 153, 150, 149, 146, 185, 166, 164, 167, 186, 170, 169, 171, 187, 177, 176, 178, 165, 195, 194, 188, 181, 180, 182, 189, 191, 228, 168, 172, 145, 179, 183, 142, 233, 232, 198, 200, 201, 202, -1, -1, -1, -1, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 192, 156, 157, 133, -1, 148, 131, -1, -1, -1, -1, -1, -1, -1, 226, -1, 199, -1, 159, 158, 138, 224, 225, -1, -1, -1, -1, -1, -1, -1, 227, -1, -1, -2, 223, 229, -1, 132, -1, -1, -1, 184, 196, -1, 143, 160, 161, 162, 163, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 155, 197};

#define IRMAPCASE(x) case x: return  _##x [item->getAuxValue() & 15]

int ItemRenderer::getAtlasPos(const ItemInstance* item) {
	int id = item->id;
	if (id < 0 || id >= sizeof(_mapper) / sizeof(const signed short))
		return -1;

	int texId = _mapper[id];
	if (texId != -2)
		return texId;

	switch(id) {
		IRMAPCASE(6);
		IRMAPCASE(17);
		IRMAPCASE(18);
		IRMAPCASE(24);
		IRMAPCASE(35);
		IRMAPCASE(44);
		IRMAPCASE(98);
		IRMAPCASE(155);
		IRMAPCASE(263);
		IRMAPCASE(351);
	default:
		break;
	}
	return -1;
}

/*static*/
void ItemRenderer::renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, bool fancy) {
	renderGuiItem(font, textures, item, x, y, 16, 16, fancy);
}
void ItemRenderer::renderGuiItem(Font* font, Textures* textures, const ItemInstance* item, float x, float y, float w, float h, bool fancy) {
	if (item == NULL) {
		//LOGW("item is NULL @ ItemRenderer::renderGuiItem\n");
		return;
	}
	const int id = item->id;
	if (!Item::items[id])
		return;

	int i = getAtlasPos(item);

	if (i < 0) {
		Tesselator& t = Tesselator::instance;
		if (!t.isOverridden())
			renderGuiItemCorrect(font, textures, item, int(x), int(y));
		else {
			// @huge @attn @todo @fix:	This is just guess-works..
			//							it we're batching for saving the
			//							buffer, this will fail miserably
			t.endOverrideAndDraw();
			glDisable2(GL_TEXTURE_2D);
			fillRect(t, x, y, w, h, 0xff0000);
			glEnable2(GL_TEXTURE_2D);
			renderGuiItemCorrect(font, textures, item, int(x), int(y));
			t.beginOverride();
		}
		return;
	}

	textures->loadAndBindTexture("gui/gui_blocks.png");
	float u0, u1, v0, v1;
	if (i < 128) {
		const float P = 48.0f / 512.0f;
		u0 = (float)(i%10) * P;
		v0 = (float)(i/10) * P;
		u1 = u0 + P;
		v1 = v0 + P;
	} else {
		i -= 128;
		const float P = 16.0f / 512.0f;
		u0 = float(i & 31) * P;
		v0 = 27 * P + float(i >> 5) * P; // 27 "icon" rows down
		u1 = u0 + P;
		v1 = v0 + P;
	}

	const float blitOffset = 0;
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.colorABGR( item->count>0? 0xffffffff : 0x60ffffff);
	t.vertexUV(x,     y + h, blitOffset, u0, v1);
	t.vertexUV(x + w, y + h, blitOffset, u1, v1);
	t.vertexUV(x + w, y,     blitOffset, u1, v0);
	t.vertexUV(x,     y,     blitOffset, u0, v0);
	t.draw();
}

void ItemRenderer::renderGuiItemDecorations(const ItemInstance* item, float x, float y) {
	if (!item) return;
	if (item->count > 0 && item->isDamaged()) {
		float p = std::floor(13.5f - (float) item->getDamageValue() * 13.0f / (float) item->getMaxDamage());
		int cc = (int) std::floor(255.5f - (float) item->getDamageValue() * 255.0f / (float) item->getMaxDamage());
		//glDisable(GL_LIGHTING);
		//glDisable(GL_DEPTH_TEST);
		//glDisable(GL_TEXTURE_2D);

		Tesselator& t = Tesselator::instance;

		int ca = (255 - cc) << 16 | (cc) << 8;
		int cb = ((255 - cc) / 4) << 16 | (255 / 4) << 8;
		fillRect(t, x + 2, y + 13, 13, 1, 0x000000);
		fillRect(t, x + 2, y + 13, 12, 1, cb);
		fillRect(t, x + 2, y + 13, p, 1, ca);

		//glEnable(GL_TEXTURE_2D);
		//glEnable(GL_LIGHTING);
		//glEnable(GL_DEPTH_TEST);
		glColor4f2(1, 1, 1, 1);
	}
}

void ItemRenderer::fillRect(Tesselator& t, float x, float y, float w, float h, int c) {
	t.begin();
	t.color(c);
	t.vertex(x + 0, y + 0, 0);
	t.vertex(x + 0, y + h, 0);
	t.vertex(x + w, y + h, 0);
	t.vertex(x + w, y + 0, 0);
	t.draw();
}


void ItemRenderer::renderGuiItemCorrect(Font* font, Textures* textures, const ItemInstance* item, int x, int y) {
	if (item == NULL)
		return;

	//glDisable(GL_CULL_FACE);
	if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape()))
	{
		int paint = item->id;
		textures->loadAndBindTexture("terrain.png");

		static float ff = 0;// ff += 0.005f;
		static float gg = 0;// gg += 0.01f;

		Tile* tile = Tile::tiles[paint];
		glPushMatrix2();
		glTranslatef2((GLfloat)(x - 2), (GLfloat)(y + 3), -8);
		glScalef2(10.0f, 10.0f, 10.0f);
		glTranslatef2(1.0f, 0.5f, 0.0f);
		glRotatef2(ff + 180.0f + 30.0f, 1, 0, 0);
		glRotatef2(gg + 45.0f, 0, 1, 0);

		//glColor4f2(1, 1, 1, 1);
		glScalef2(1, 1, 1);
		tileRenderer->renderGuiTile(tile, item->getAuxValue());
		glPopMatrix2();
	}
	else if (item->getIcon() >= 0)
	{
		//if (item->id == Item::camera->id) {
		//	printf("item->id: %d, %d\n", item->id, item->getIcon());
		//}
		if (item->id < 256) {
			textures->loadAndBindTexture("terrain.png");
		} else {
			textures->loadAndBindTexture("gui/items.png");
		}
		//Tesselator& t = Tesselator::instance;
		//t.scale2d(Gui::InvGuiScale, Gui::InvGuiScale);
		blit((float)x, (float)y, (float)(item->getIcon() % 16 * 16), (float)(item->getIcon() / 16 * 16), 16, 16);
		//t.resetScale();
	}
	//glEnable(GL_CULL_FACE);
}

/*static*/
void ItemRenderer::blit(float x, float y, float sx, float sy, float w, float h) {
	float blitOffset = 0;
	const float us = 1 / 256.0f;
	const float vs = 1 / 256.0f;
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(x, y + h, blitOffset, sx * us, (sy + h) * vs);
	t.vertexUV(x + w, y + h, blitOffset, (sx + w) * us, (sy + h) * vs);
	t.vertexUV(x + w, y, blitOffset, (sx + w) * us, sy * vs);
	t.vertexUV(x, y, blitOffset, sx * us, sy * vs);
	//t.end();
	t.draw();
}
