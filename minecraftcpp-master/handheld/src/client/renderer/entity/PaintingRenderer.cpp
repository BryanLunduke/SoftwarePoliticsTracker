#include "PaintingRenderer.h"
#include "../../../world/entity/Painting.h"
#include "../../../world/Direction.h"
#include "../Tesselator.h"
#include "../Textures.h"
#include "../gles.h"

void PaintingRenderer::render( Entity* entity, float x, float y, float z, float rot, float a ) {
	glPushMatrix();
	glTranslatef(float(x), float(y), float(z));
	glRotatef(rot, 0, 1.0f, 0);
	//glEnable(GL_RESCALE_NORMAL);
	bindTexture("art/kz.png");
	Painting* painting = (Painting*)entity;
	const Motive* motive = painting->motive;

	float s = 1.0f / 16.0f;
	glScalef(s, s, s);
	renderPainting(painting, motive->w, motive->h, motive->uo, motive->vo, a);
	//glDisable(GL_RESCALE_NORMAL);
	glPopMatrix();
}

void PaintingRenderer::renderPainting( Painting* painting, int w, int h, int uo, int vo, float a) {
	float xx0 = -w / 2.0f;
	float yy0 = -h / 2.0f;

	float edgeWidth = 0.5f;

	// Back
	float bu0 = (12 * 16) / 256.0f;
	float bu1 = (12 * 16 + 16) / 256.0f;
	float bv0 = (0) / 256.0f;
	float bv1 = (0 + 16) / 256.0f;

	// Border
	float uu0 = (12 * 16) / 256.0f;
	float uu1 = (12 * 16 + 16) / 256.0f;
	float uv0 = (0.5f) / 256.0f;
	float uv1 = (0.5f) / 256.0f;

	// Border
	float su0 = (12 * 16 + 0.5f) / 256.0f;
	float su1 = (12 * 16 + 0.5f) / 256.0f;
	float sv0 = (0) / 256.0f;
	float sv1 = (0 + 16) / 256.0f;

	for (int xs = 0; xs < w / 16; xs++) {
		for (int ys = 0; ys < h / 16; ys++) {
			float x0 = xx0 + (xs + 1) * 16;
			float x1 = xx0 + (xs) * 16;
			float y0 = yy0 + (ys + 1) * 16;
			float y1 = yy0 + (ys) * 16;
			//setBrightness(painting, (x0 + x1) / 2, (y0 + y1) / 2);

			// Painting
			float fu0 = (uo + w - (xs) * 16) / 256.0f;
			float fu1 = (uo + w - (xs + 1) * 16) / 256.0f;
			float fv0 = (vo + h - (ys) * 16) / 256.0f;
			float fv1 = (vo + h - (ys + 1) * 16) / 256.0f;

			Tesselator& t = Tesselator::instance;
			float br = painting->getBrightness(a);
			t.color(br, br, br);

			t.begin();
			t.vertexUV(x0, y1, -edgeWidth, fu1, fv0);
			t.vertexUV(x1, y1, -edgeWidth, fu0, fv0);
			t.vertexUV(x1, y0, -edgeWidth, fu0, fv1);
			t.vertexUV(x0, y0, -edgeWidth, fu1, fv1);

			t.vertexUV(x0, y0, edgeWidth, bu0, bv0);
			t.vertexUV(x1, y0, edgeWidth, bu1, bv0);
			t.vertexUV(x1, y1, edgeWidth, bu1, bv1);
			t.vertexUV(x0, y1, edgeWidth, bu0, bv1);

			t.vertexUV(x0, y0, -edgeWidth, uu0, uv0);
			t.vertexUV(x1, y0, -edgeWidth, uu1, uv0);
			t.vertexUV(x1, y0, edgeWidth, uu1, uv1);
			t.vertexUV(x0, y0, edgeWidth, uu0, uv1);

			t.vertexUV(x0, y1, edgeWidth, uu0, uv0);
			t.vertexUV(x1, y1, edgeWidth, uu1, uv0);
			t.vertexUV(x1, y1, -edgeWidth, uu1, uv1);
			t.vertexUV(x0, y1, -edgeWidth, uu0, uv1);

			t.vertexUV(x0, y0, edgeWidth, su1, sv0);
			t.vertexUV(x0, y1, edgeWidth, su1, sv1);
			t.vertexUV(x0, y1, -edgeWidth, su0, sv1);
			t.vertexUV(x0, y0, -edgeWidth, su0, sv0);

			t.vertexUV(x1, y0, -edgeWidth, su1, sv0);
			t.vertexUV(x1, y1, -edgeWidth, su1, sv1);
			t.vertexUV(x1, y1, edgeWidth, su0, sv1);
			t.vertexUV(x1, y0, edgeWidth, su0, sv0);
			t.draw();
		}
	}
}