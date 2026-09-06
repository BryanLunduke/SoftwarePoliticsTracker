#include "ItemInHandRenderer.h"

#include "gles.h"
#include "Tesselator.h"
#include "entity/EntityRenderDispatcher.h"
#include "entity/EntityRenderer.h"
#include "entity/MobRenderer.h"
#include "../Minecraft.h"
#include "../player/LocalPlayer.h"
#include "../../world/entity/player/Player.h"
#include "../../world/item/Item.h"
#include "../../world/level/material/Material.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/Level.h"
#include "../../util/Mth.h"
#include "../../world/entity/player/Inventory.h"

#include "../../platform/time.h"
#include "Textures.h"
#include "../../world/item/UseAnim.h"
#include "../../world/item/BowItem.h"
#include "../../world/level/tile/LeafTile.h"
#include "entity/HumanoidMobRenderer.h"

//static StopwatchHandler handler;

ItemInHandRenderer::ItemInHandRenderer( Minecraft* mc )
:	mc(mc),
	lastSlot(-1),
	height(0),
	oHeight(0),
	lastIconRendered(0),
	lastItemRendered(0),
	//selectedItem(NULL),
	item(0, 1, 0)
{
	GLuint ids[MaxNumRenderObjects];
	glGenBuffers2(MaxNumRenderObjects, ids);

	for (int i = 0; i < MaxNumRenderObjects; ++i) {
		renderObjects[i].itemId = -1;
		renderObjects[i].chunk.vboId = ids[i];
		//LOGI("IDS: %d\n", ids[i]);
	}

	//LOGI("SizeOf :%d (256 * %d)\n", sizeof(renderObjects), sizeof(RenderCall));
}

void ItemInHandRenderer::tick()
{
	oHeight = height;
	item.id = 0;

	ItemInstance* itemInHand = mc->player->inventory->getSelected();
	if (itemInHand && itemInHand->count > 0) {
		item.id = itemInHand->id;
		item.setAuxValue(itemInHand->getAuxValue());
	}

	float max = 0.4f;
	float tHeight = 1;//matches ? 1 : 0;
	float dd = tHeight - height;
	if (dd < -max) dd = -max;
	if (dd > max) dd = max;

	height += dd;
}

void ItemInHandRenderer::renderItem(Mob* mob,  ItemInstance* item )
{
	//Stopwatch& w = handler.get("item:" + Tile::tiles[item->id]->getDescriptionId());
	//w.start();

	int itemId = item->id;
	// Ugly hack, but what the heck, I don't have time at the moment
	// Use spare slots between 200 and 255 (for now) for items with different
	// graphics per data. @note: Check Tile.cpp for needed IDs to override
	if (itemId == Tile::cloth->id) {
		itemId = 200 + item->getAuxValue(); // 200 to 215
	} else if (itemId == Tile::treeTrunk->id) {
		itemId = 216 + item->getAuxValue(); // 216 to 219 @treeTrunk
	} else if (itemId == Tile::sapling->id) {
		itemId = 220 + item->getAuxValue(); // 220 to 223 @sapling
	} else if (itemId == Tile::stoneSlabHalf->id) {
		itemId = 224 + item->getAuxValue(); // 224 to 231 @stoneslab
	} else if (itemId == ((Tile*)Tile::leaves)->id) {
		itemId = 232 + (item->getAuxValue() & LeafTile::LEAF_TYPE_MASK); // 232 to 235 @leaves
	}

	RenderCall& renderObject = renderObjects[itemId];
	int itemIcon = item->getIcon();
	if(mob != NULL) {
		itemIcon = mob->getItemInHandIcon(item, 0);
	}
	
	bool reTesselate(false);
	if(itemIcon != lastIconRendered && lastItemRendered == itemId)
		reTesselate  = true;
	lastItemRendered = itemId;
	lastIconRendered = itemIcon;
	//const int aux = item->getAuxValue();

	if (renderObject.itemId == -1 || reTesselate) {
		if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
			Tesselator& t = Tesselator::instance;
			t.beginOverride();

			int renderedItemId = item->id;
			if (renderedItemId == ((Tile*)Tile::leaves)->id)
				renderedItemId = ((Tile*)Tile::leaves_carried)->id;
			if (renderedItemId == Tile::grass->id)
				renderedItemId = Tile::grass_carried->id;
			tileRenderer.renderTile(Tile::tiles[renderedItemId], item->getAuxValue());
			renderObject.chunk = t.endOverride(renderObject.chunk.vboId);

			renderObject.texture = "terrain.png";
			renderObject.itemId = itemId;
			renderObject.isFlat = false;
		} else {
			renderObject.itemId = itemId;
			renderObject.isFlat = true;

			if (item->id < 256) {
				renderObject.texture = "terrain.png";
				//mc->textures->loadAndBindTexture("terrain.png");
			} else {
				renderObject.texture = "gui/items.png";
				//mc->textures->loadAndBindTexture("gui/items.png");
			}
			// glDisable2(GL_LIGHTING);
			Tesselator& t = Tesselator::instance;
			t.beginOverride();
			t.color(0xff, 0xff, 0xff);
			const int up = itemIcon & 15;
			const int vp = itemIcon >> 4;
			float u1 = (up * 16 + 0.00f) / 256.0f;
			float u0 = (up * 16 + 15.99f) / 256.0f;
			float v0 = (vp * 16 + 0.00f) / 256.0f;
			float v1 = (vp * 16 + 15.99f) / 256.0f;

			float r = 1.0f;
//			float xo = 0.0f;
//			float yo = 0.3f;
/*
			//glEnable2(GL_RESCALE_NORMAL);
			glTranslatef2(-xo, -yo, 0);
			float s = 1.5f;
			glScalef2(s, s, s);

			glRotatef2(50, 0, 1, 0);
			glRotatef2(45 + 290, 0, 0, 1);
			glTranslatef2(-15 / 16.0f, -1 / 16.0f, 0);
*/
			float dd = 1 / 16.0f;

			t.vertexUV(0, 0, 0, u0, v1);
			t.vertexUV(r, 0, 0, u1, v1);
			t.vertexUV(r, 1, 0, u1, v0);
			t.vertexUV(0, 1, 0, u0, v0);

			t.vertexUV(0, 1, 0 - dd, u0, v0);
			t.vertexUV(r, 1, 0 - dd, u1, v0);
			t.vertexUV(r, 0, 0 - dd, u1, v1);
			t.vertexUV(0, 0, 0 - dd, u0, v1);

			for (int i = 0; i < 16; i++) {
				float p = i / 16.0f;
				float uu = u0 + (u1 - u0) * p - 0.5f / 256.0f;
				float xx = r * p;
				t.vertexUV(xx, 0, 0 - dd, uu, v1);
				t.vertexUV(xx, 0, 0, uu, v1);
				t.vertexUV(xx, 1, 0, uu, v0);
				t.vertexUV(xx, 1, 0 - dd, uu, v0);
			}
			for (int i = 0; i < 16; i++) {
				float p = i / 16.0f;
				float uu = u0 + (u1 - u0) * p - 0.5f / 256.0f;
				float xx = r * p + 1 / 16.0f;
				t.vertexUV(xx, 1, 0 - dd, uu, v0);
				t.vertexUV(xx, 1, 0, uu, v0);
				t.vertexUV(xx, 0, 0, uu, v1);
				t.vertexUV(xx, 0, 0 - dd, uu, v1);
			}
			for (int i = 0; i < 16; i++) {
				float p = i / 16.0f;
				float vv = v1 + (v0 - v1) * p - 0.5f / 256.0f;
				float yy = r * p + 1 / 16.0f;
				t.vertexUV(0, yy, 0, u0, vv);
				t.vertexUV(r, yy, 0, u1, vv);
				t.vertexUV(r, yy, 0 - dd, u1, vv);
				t.vertexUV(0, yy, 0 - dd, u0, vv);
			}
			for (int i = 0; i < 16; i++) {
				float p = i / 16.0f;
				float vv = v1 + (v0 - v1) * p - 0.5f / 256.0f;
				float yy = r * p;
				t.vertexUV(r, yy, 0, u1, vv);
				t.vertexUV(0, yy, 0, u0, vv);
				t.vertexUV(0, yy, 0 - dd, u0, vv);
				t.vertexUV(r, yy, 0 - dd, u1, vv);
			}
			renderObject.chunk = t.endOverride(renderObject.chunk.vboId);
		}
	}

	if (renderObject.itemId >= 0) {
		if (renderObject.isFlat) {
			float xo = 0.0f;
			float yo = 0.3f;

			glPushMatrix2();
			glTranslatef2(-xo, -yo, 0);
			float s = 1.5f;
			glScalef2(s, s, s);

			glRotatef2(50, 0, 1, 0);
			glRotatef2(45 + 290, 0, 0, 1);
			glTranslatef2(-15 / 16.0f, -1 / 16.0f, 0);
		}
		mc->textures->loadAndBindTexture(renderObject.texture);

		drawArrayVT_NoState(renderObject.chunk.vboId, renderObject.chunk.vertexCount);
		if (renderObject.isFlat)
			glPopMatrix2();
	}
	//w.stop();
	//handler.printEvery(100);
}

void ItemInHandRenderer::render( float a )
{
	//return;

	//Stopwatch& w = handler.get("render");
	//w.start();

	float h = oHeight + (height - oHeight) * a;
	Player* player = mc->player;
	// if (selectedTile==NULL) return;

	glPushMatrix2();
	glRotatef2(player->xRotO + (player->xRot - player->xRotO) * a, 1, 0, 0);
	glRotatef2(player->yRotO + (player->yRot - player->yRotO) * a, 0, 1, 0);
	glPopMatrix2();

	float br = mc->level->getBrightness(Mth::floor(player->x), Mth::floor(player->y), Mth::floor(player->z));

	ItemInstance* item;// = selectedItem;
	//if (player.fishing != NULL) {
	//    item = /*new*/ ItemInstance(Item.stick);
	//}

	if (this->item.id > 0)
		item = &this->item;
	else
		item = NULL;

	if (item != NULL) {
		glColor4f2(br, br, br, 1);
		bool isUsing = player->getUseItemDuration() > 0;
		bool isBow = item->id == Item::bow->id;
		bool noSwingAnimation = isUsing && (isBow || (item->getUseAnimation() != UseAnim::none));
		const float swing = noSwingAnimation ? 0 : player->getAttackAnim(a);
		const float sqrtSwing = Mth::sqrt(swing);
		const float swing3 = Mth::sin(swing * swing * Mth::PI);
		const float swing1 = Mth::sin(swing * Mth::PI);
		const float swing2 = Mth::sin(sqrtSwing * Mth::PI);
		const float d = 0.8f;
		glPushMatrix2();

		if (isUsing) {
			UseAnim::UseAnimation anim = item->getUseAnimation();
			if (anim == UseAnim::eat || anim == UseAnim::drink) {
				float t = (player->getUseItemDuration() - a + 1);
				float useProgress = 1 - (t / item->getUseDuration());

				float swing = useProgress;
				float is = 1 - swing;
				is = is * is * is;
				is = is * is * is;
				is = is * is * is;
				float iss = 1 - is;
				glTranslatef(0, Mth::abs(Mth::cos(t / 4 * Mth::PI) * 0.1f) * (swing > 0.2 ? 1 : 0), 0);
				glTranslatef(iss * 0.6f, -iss * 0.5f, 0);
				glRotatef(iss * 90, 0, 1, 0);
				glRotatef(iss * 10, 1, 0, 0);
				glRotatef(iss * 30, 0, 0, 1);
			}
		} else {
			glTranslatef2(-swing2 * 0.4f, (float) Mth::sin(sqrtSwing * Mth::PI * 2) * 0.2f, -swing1 * 0.2f);
		}

		glTranslatef2(0.7f * d, -0.65f * d - (1 - h) * 0.6f, -0.9f * d);

		glRotatef2(45, 0, 1, 0);
		//glEnable2(GL_RESCALE_NORMAL);
		glRotatef2(-swing3 * 20, 0, 1, 0);
		glRotatef2(-swing2 * 20, 0, 0, 1);
		glRotatef2(-swing2 * 80, 1, 0, 0);
		float ss = 0.4f;
		glScalef2(ss, ss, ss);
		if (player->getUseItemDuration() > 0) {
			UseAnim::UseAnimation anim = item->getUseAnimation();
			if(anim == UseAnim::bow) {
				glRotatef(-18, 0, 0, 1);
				glRotatef(-12, 0, 1, 0);
				glRotatef(-8, 1, 0, 0);
				glTranslatef(-0.9f, 0.2f, 0.0f);
				float timeHeld = (item->getUseDuration() - (player->getUseItemDuration() - a + 1));
				float pow = timeHeld / (float) (BowItem::MAX_DRAW_DURATION);
				pow = ((pow * pow) + pow * 2) / 3;
				if (pow > 1) pow = 1;
				if (pow > 0.1f) {
					glTranslatef(0, Mth::sin((timeHeld - 0.1f) * 1.3f) * 0.01f * (pow - 0.1f), 0);
				}
				glTranslatef(0, 0, pow * 0.1f);

				glRotatef(-45 - 290, 0, 0, 1);
				glRotatef(-50, 0, 1, 0);
				glTranslatef(0, 0.5f, 0);
				float ys = 1 + pow * 0.2f;
				glScalef(1, 1, ys);
				glTranslatef(0, -0.5f, 0);
				glRotatef(50, 0, 1, 0);
				glRotatef(45 + 290, 0, 0, 1);
			}
		}
		if (item->getItem()->isMirroredArt()) {
			glRotatef2(180, 0, 1, 0);
		}
		glEnableClientState2(GL_VERTEX_ARRAY);
		glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
		renderItem(player, item);
		glDisableClientState2(GL_VERTEX_ARRAY);
		glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
		glPopMatrix2();
	} else {
		glColor4f2(br, br, br, 1);

		glPushMatrix2();
		float d = 0.8f;
		const float swing = player->getAttackAnim(a);
		const float sqrtSwing = Mth::sqrt(swing);
		const float swing3 = Mth::sin(swing * swing * Mth::PI);
		const float swing1 = Mth::sin(swing * Mth::PI);
		const float swing2 = Mth::sin(sqrtSwing * Mth::PI);

		glTranslatef2(-swing2 * 0.3f, (float) Mth::sin(Mth::sqrt(swing) * Mth::PI * 2) * 0.4f, -swing1 * 0.4f);
		glTranslatef2(0.8f * d, -0.75f * d - (1 - h) * 0.6f, -0.9f * d);

		glRotatef2(45, 0, 1, 0);
		//glEnable2(GL_RESCALE_NORMAL);
		glRotatef2(swing2 * 70, 0, 1, 0);
		glRotatef2(-swing3 * 20, 0, 0, 1);
		// glRotatef2(-swing2 * 80, 1, 0, 0);

		mc->textures->loadAndBindTexture("mob/char.png");
		glTranslatef2(-1.0f, +3.6f, +3.5f);
		glRotatef2(120, 0, 0, 1);
		glRotatef2(180 + 20, 1, 0, 0);
		glRotatef2(-90 - 45, 0, 1, 0);
		glScalef2(1.5f / 24.0f * 16, 1.5f / 24.0f * 16, 1.5f / 24.0f * 16);
		glTranslatef2(5.6f, 0, 0);

		EntityRenderer* er = EntityRenderDispatcher::getInstance()->getRenderer(mc->player);
		HumanoidMobRenderer* playerRenderer = (HumanoidMobRenderer*) er;
		float ss = 1;
		glScalef2(ss, ss, ss);
		playerRenderer->renderHand();
		glPopMatrix2();
	}
	//glDisable2(GL_RESCALE_NORMAL);
	//Lighting.turnOff();
	//w.stop();
}

void ItemInHandRenderer::renderScreenEffect( float a )
{
	glDisable2(GL_ALPHA_TEST);
	if (mc->player->isOnFire()) {
		mc->textures->loadAndBindTexture("terrain.png");
		renderFire(a);
	}

	if (mc->player->isInWall()) // Inside a tile
	{
		int x = Mth::floor(mc->player->x);
		int y = Mth::floor(mc->player->y);
		int z = Mth::floor(mc->player->z);

		mc->textures->loadAndBindTexture("terrain.png");
		int tile = mc->level->getTile(x, y, z);
		if (Tile::tiles[tile] != NULL) {
			renderTex(a, Tile::tiles[tile]->getTexture(2));
		}
	}

	//     if (mc->player->isUnderLiquid(Material::water)) {
	//mc->textures->loadAndBindTexture("misc/water.png");
	//         renderWater(a);
	//     }
	glEnable2(GL_ALPHA_TEST);
}

void ItemInHandRenderer::itemPlaced()
{
	height = 0;
}

void ItemInHandRenderer::itemUsed()
{
	height = 0;
}

void ItemInHandRenderer::renderTex( float a, int tex )
{
	Tesselator& t = Tesselator::instance;

	float br;// = mc->player->getBrightness(a);
	br = 0.1f;
	glColor4f2(br, br, br, 0.5f);

	glPushMatrix2();

	float x0 = -1;
	float x1 = +1;
	float y0 = -1;
	float y1 = +1;
	float z0 = -0.5f;

	float r = 2 / 256.0f;
	float u0 = (tex % 16) / 256.0f - r;
	float u1 = (tex % 16 + 15.99f) / 256.0f + r;
	float v0 = (tex / 16) / 256.0f - r;
	float v1 = (tex / 16 + 15.99f) / 256.0f + r;

	t.begin();
	t.vertexUV(x0, y0, z0, u1, v1);
	t.vertexUV(x1, y0, z0, u0, v1);
	t.vertexUV(x1, y1, z0, u0, v0);
	t.vertexUV(x0, y1, z0, u1, v0);
	//t.end();
	t.draw();
	glPopMatrix2();

	glColor4f2(1, 1, 1, 1);
}

void ItemInHandRenderer::renderWater( float a )
{
	Tesselator& t = Tesselator::instance;

	float br = mc->player->getBrightness(a);
	glColor4f2(br, br, br, 0.5f);
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glPushMatrix2();

	float size = 4;

	float x0 = -1;
	float x1 = +1;
	float y0 = -1;
	float y1 = +1;
	float z0 = -0.5f;

	float uo = -mc->player->yRot / 64.0f;
	float vo = +mc->player->xRot / 64.0f;

	t.begin();
	t.vertexUV(x0, y0, z0, size + uo, size + vo);
	t.vertexUV(x1, y0, z0, 0 + uo, size + vo);
	t.vertexUV(x1, y1, z0, 0 + uo, 0 + vo);
	t.vertexUV(x0, y1, z0, size + uo, 0 + vo);
	//t.end();
	t.draw();
	glPopMatrix2();

	glColor4f2(1, 1, 1, 1);
	glDisable2(GL_BLEND);
}

void ItemInHandRenderer::renderFire( float a )
{
	Tesselator& t = Tesselator::instance;
	glColor4f2(1, 1, 1, 0.9f);
	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float size = 1;
	for (int i = 0; i < 2; i++) {
		glPushMatrix2();
		int tex = ((Tile*)Tile::fire)->tex + i * 16;
		int xt = (tex & 0xf) << 4;
		int yt = tex & 0xf0;

		float u0 = (xt) / 256.0f;
		float u1 = (xt + 15.99f) / 256.0f;
		float v0 = (yt) / 256.0f;
		float v1 = (yt + 15.99f) / 256.0f;

		float x0 = (0 - size) / 2;
		float x1 = x0 + size;
		float y0 = 0 - size / 2;
		float y1 = y0 + size;
		float z0 = -0.5f;
		glTranslatef2(-(i * 2 - 1) * 0.24f, -0.3f, 0);
		glRotatef2((i * 2 - 1) * 10.f, 0, 1, 0);

		t.begin();
		t.vertexUV(x0, y0, z0, u1, v1);
		t.vertexUV(x1, y0, z0, u0, v1);
		t.vertexUV(x1, y1, z0, u0, v0);
		t.vertexUV(x0, y1, z0, u1, v0);
		//t.end();
		t.draw();
		glPopMatrix2();
	}
	glColor4f2(1, 1, 1, 1);
	glDisable2(GL_BLEND);
}

void ItemInHandRenderer::onGraphicsReset()
{
	GLuint ids[MaxNumRenderObjects];
	glGenBuffers2(MaxNumRenderObjects, ids);

	for (int i = 0; i < MaxNumRenderObjects; ++i) {
		renderObjects[i].itemId = -1;
		renderObjects[i].chunk.vboId = ids[i];
	}
}
