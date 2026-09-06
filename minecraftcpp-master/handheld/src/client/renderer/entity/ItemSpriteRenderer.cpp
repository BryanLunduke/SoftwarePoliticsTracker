#include "ItemSpriteRenderer.h"
#include "EntityRenderDispatcher.h"
#include "../Tesselator.h"
#include "../../../world/entity/Entity.h"


ItemSpriteRenderer::ItemSpriteRenderer( int icon )
:	icon(icon)
{
}

void ItemSpriteRenderer::render( Entity* e, float x, float y, float z, float rot, float a )
{
	glPushMatrix2();

	glTranslatef2((float) x, (float) y, (float) z);
	glScalef2(1 / 2.0f, 1 / 2.0f, 1 / 2.0f);
	bindTexture("gui/items.png");
	Tesselator& t = Tesselator::instance;

	float u0 = ((icon % 16) * 16 + 0) / 256.0f;
	float u1 = ((icon % 16) * 16 + 16) / 256.0f;
	float v0 = ((icon / 16) * 16 + 0) / 256.0f;
	float v1 = ((icon / 16) * 16 + 16) / 256.0f;


	float r = 1.0f;
	float xo = 0.5f;
	float yo = 0.25f;

	glRotatef2(180 - entityRenderDispatcher->playerRotY, 0, 1, 0);
	glRotatef2(-entityRenderDispatcher->playerRotX, 1, 0, 0);
	t.begin();
	t.vertexUV(0 - xo, 0 - yo, 0, u0, v1);
	t.vertexUV(r - xo, 0 - yo, 0, u1, v1);
	t.vertexUV(r - xo, 1 - yo, 0, u1, v0);
	t.vertexUV(0 - xo, 1 - yo, 0, u0, v0);
	t.draw();

	glPopMatrix2();
}
