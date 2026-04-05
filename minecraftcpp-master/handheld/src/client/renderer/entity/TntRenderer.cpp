#include "TntRenderer.h"
#include "../Tesselator.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/entity/item/PrimedTnt.h"


TntRenderer::TntRenderer()
{
	this->shadowRadius = 0.5f;
}

void TntRenderer::render( Entity* tnt_, float x, float y, float z, float rot, float a )
{
	PrimedTnt* tnt = (PrimedTnt*)tnt_;
	glPushMatrix2();
	glTranslatef2((float) x, (float) y, (float) z);
	if (tnt->life - a + 1 < 10) {
		float g = 1 - ((tnt->life - a + 1) / 10.0f);
		if (g < 0) g = 0;
		if (g > 1) g = 1;
		g = g * g;
		g = g * g;
		float s = 1.0f + g * 0.3f;
		glScalef2(s, s, s);
	}

	float br = (1 - ((tnt->life - a + 1) / 100.0f)) * 0.8f;
	bindTexture("terrain.png");

	Tesselator& t = Tesselator::instance;
	t.color(1.0f, 1.0f, 1.0f);
	tileRenderer.renderTile(Tile::tnt, 0);

	if (((tnt->life / 5) & 1) == 0) {
		glDisable2(GL_TEXTURE_2D);
		glEnable2(GL_BLEND);
		glBlendFunc2(GL_SRC_ALPHA, GL_DST_ALPHA);
		glColor4f2(1, 1, 1, br);
		//t.color(1.0f, 1.0f, 1.0f, br);
		tileRenderer.renderTile(Tile::tnt, 0);
		glColor4f2(1, 1, 1, 1);
		glDisable2(GL_BLEND);
		//glEnable2(GL_LIGHTING);
		glEnable2(GL_TEXTURE_2D);
	}
	glPopMatrix2();
}
