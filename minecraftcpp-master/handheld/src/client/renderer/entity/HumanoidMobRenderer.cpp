#include "HumanoidMobRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../ItemInHandRenderer.h"
#include "../TileRenderer.h"
#include "../../model/HumanoidModel.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/entity/player/Player.h"
#include "../../../world/entity/player/Inventory.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/item/Item.h"
#include "../../../world/item/BowItem.h"

HumanoidMobRenderer::HumanoidMobRenderer(HumanoidModel* humanoidModel, float shadow)
:	super(humanoidModel, shadow),
	humanoidModel(humanoidModel)
{
}

void HumanoidMobRenderer::renderHand() {
	humanoidModel->attackTime = 0;
	humanoidModel->setupAnim(0, 0, 0, 0, 0, 1 / 16.0f);

	//@attn @cuberender @enableClientState @vertexarray
	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	//glEnableClientState2(GL_COLOR_ARRAY); 
	humanoidModel->arm0.render(1 / 16.0f);
	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
	//glDisableClientState2(GL_COLOR_ARRAY); 
}

void HumanoidMobRenderer::additionalRendering(Mob* mob, float a) {
	ItemInstance* item = mob->getCarriedItem();
	if (item != NULL && item->count > 0) {
		glPushMatrix2();
		humanoidModel->arm0.translateTo(1 / 16.0f);
		glTranslatef2(-1.0f / 16.0f, 7.0f / 16.0f, 1.0f / 16.0f);

		if (item->id < 256 && TileRenderer::canRender(Tile::tiles[item->id]->getRenderShape())) {
			float s = 8.0f / 16.0f;
			glTranslatef2(0.0f, 3.0f / 16.0f, -5 / 16.0f);
			s *= 0.75f;
			glRotatef2(20.0f, 1.0f, 0.0f, 0.0f);
			glRotatef2(45.0f, 0.0f, 1.0f, 0.0f);
			glScalef2(s, -s, s);
		} else if (item->id == Item::bow->id) {
			const float s = 10.0f / 16.0f;
			glTranslatef2(0 / 16.0f, 2 / 16.0f, 5 / 16.0f);
			glRotatef2(-20, 0, 1, 0);
			glScalef2(s, -s, s);
			glRotatef2(-100, 1, 0, 0);
			glRotatef2(45, 0, 1, 0);
		} else if (Item::items[item->id]->isHandEquipped()) {
			float s = 10.0f / 16.0f;
			glTranslatef2(0.0f, 3.0f / 16.0f, 0.0f);
			glScalef2(s, -s, s);
			glRotatef2(-100.0f, 1.0f, 0.0f, 0.0f);
			glRotatef2(45.0f, 0.0f, 1.0f, 0.0f);
		} else {
			float s = 6 / 16.0f;
			glTranslatef2(+4 / 16.0f, +3 / 16.0f, -3 / 16.0f);
			glScalef2(s, s, s);
			glRotatef2(60.0f, 0.0f, 0.0f, 1.0f);
			glRotatef2(-90.0f, 1.0f, 0.0f, 0.0f);
			glRotatef2(20.0f, 0.0f, 0.0f, 1.0f);
		}
		entityRenderDispatcher->itemInHandRenderer->renderItem(mob, item);
		glPopMatrix2();
	}
}

void HumanoidMobRenderer::render( Entity* mob_, float x, float y, float z, float rot, float a ) {
	Mob* mob = (Mob*)mob_;
	ItemInstance* carriedItem = mob->getCarriedItem();
	if(carriedItem != NULL)
		humanoidModel->holdingRightHand = true;

	humanoidModel->sneaking = mob->isSneaking();

	super::render(mob_, x, y, z, rot, a);
	humanoidModel->holdingRightHand = false;
}
