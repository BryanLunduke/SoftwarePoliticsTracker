#include "GuiComponent.h"

#include "../renderer/Tesselator.h"
#include "../renderer/gles.h"
#include "Font.h"


GuiComponent::GuiComponent()
:	blitOffset(0)
{
}

GuiComponent::~GuiComponent()
{
}

void GuiComponent::drawCenteredString( Font* font, const std::string& str, int x, int y, int color )
{
	font->drawShadow(str, (float)(x - font->width(str) / 2), (float)(y - font->height(str) / 2), color);
}

void GuiComponent::drawString( Font* font, const std::string& str, int x, int y, int color )
{
	font->drawShadow(str, (float)x, (float)y /*- font->height(str)/2*/, color);
}

void GuiComponent::blit( int x, int y, int sx, int sy, int w, int h, int sw/*=0*/, int sh/*=0*/ )
{
	if (!sw) sw = w;
	if (!sh) sh = h;
	float us = 1 / 256.0f;
	float vs = 1 / 256.0f;
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV((float)(x)    , (float)(y + h), blitOffset, (float)(sx    ) * us, (float)(sy + sh) * vs);
	t.vertexUV((float)(x + w), (float)(y + h), blitOffset, (float)(sx + sw) * us, (float)(sy + sh) * vs);
	t.vertexUV((float)(x + w), (float)(y)    , blitOffset, (float)(sx + sw) * us, (float)(sy    ) * vs);
	t.vertexUV((float)(x)    , (float)(y)    , blitOffset, (float)(sx    ) * us, (float)(sy    ) * vs);
	t.draw();
}
void GuiComponent::blit( float x, float y, int sx, int sy, float w, float h, int sw/*=0*/, int sh/*=0*/ )
{
	if (!sw) sw = (int)w;
	if (!sh) sh = (int)h;
	float us = 1 / 256.0f;
	float vs = 1 / 256.0f;
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.vertexUV(x    , y + h, blitOffset, (float)(sx    ) * us, (float)(sy + sh) * vs);
	t.vertexUV(x + w, y + h, blitOffset, (float)(sx + sw) * us, (float)(sy + sh) * vs);
	t.vertexUV(x + w, y    , blitOffset, (float)(sx + sw) * us, (float)(sy    ) * vs);
	t.vertexUV(x    , y    , blitOffset, (float)(sx    ) * us, (float)(sy    ) * vs);
	t.draw();
}

void GuiComponent::fill( int x0, int y0, int x1, int y1, int col ) {
	fill((float)x0, (float)y0, (float)x1, (float)y1, col);
}
void GuiComponent::fill( float x0, float y0, float x1, float y1, int col )
{
	//float a = ((col >> 24) & 0xff) / 255.0f;
	//float r = ((col >> 16) & 0xff) / 255.0f;
	//float g = ((col >> 8) & 0xff) / 255.0f;
	//float b = ((col) & 0xff) / 255.0f;
	//glColor4f2(r, g, b, a);

	Tesselator& t = Tesselator::instance;
	glEnable2(GL_BLEND);
	glDisable2(GL_TEXTURE_2D);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	//LOGI("col: %f, %f, %f, %f\n", r, g, b, a);
	t.begin();
	const int color = (col&0xff00ff00) | ((col&0xff0000) >> 16) | ((col&0xff) << 16);
	t.colorABGR(color);
	t.vertex(x0, y1, 0);
	t.vertex(x1, y1, 0);
	t.vertex(x1, y0, 0);
	t.vertex(x0, y0, 0);
	t.draw();
	glEnable2(GL_TEXTURE_2D);
	glDisable2(GL_BLEND);
}

void GuiComponent::fillGradient( int x0, int y0, int x1, int y1, int col1, int col2 ) {
	fillGradient((float)x0, (float)y0, (float)x1, (float)y1, col1, col2);
}
void GuiComponent::fillGradient( float x0, float y0, float x1, float y1, int col1, int col2 )
{
	float a1 = ((col1 >> 24) & 0xff) / 255.0f;
	float r1 = ((col1 >> 16) & 0xff) / 255.0f;
	float g1 = ((col1 >> 8) & 0xff) / 255.0f;
	float b1 = ((col1) & 0xff) / 255.0f;

	float a2 = ((col2 >> 24) & 0xff) / 255.0f;
	float r2 = ((col2 >> 16) & 0xff) / 255.0f;
	float g2 = ((col2 >> 8) & 0xff) / 255.0f;
	float b2 = ((col2) & 0xff) / 255.0f;
	glDisable2(GL_TEXTURE_2D);
	glEnable2(GL_BLEND);
	glDisable2(GL_ALPHA_TEST);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel2(GL_SMOOTH);

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(r1, g1, b1, a1);
	t.vertex(x1, y0, 0);
	t.vertex(x0, y0, 0);
	t.color(r2, g2, b2, a2);
	t.vertex(x0, y1, 0);
	t.vertex(x1, y1, 0);
	t.draw();

	glShadeModel2(GL_FLAT);
	glDisable2(GL_BLEND);
	glEnable2(GL_ALPHA_TEST);
	glEnable2(GL_TEXTURE_2D);
}
void GuiComponent::fillHorizontalGradient( int x0, int y0, int x1, int y1, int col1, int col2 ) {
	fillHorizontalGradient((float)x0, (float)y0, (float)x1, (float)y1, col1, col2);
}
void GuiComponent::fillHorizontalGradient( float x0, float y0, float x1, float y1, int col1, int col2 )
{
	float a1 = ((col1 >> 24) & 0xff) / 255.0f;
	float r1 = ((col1 >> 16) & 0xff) / 255.0f;
	float g1 = ((col1 >> 8) & 0xff) / 255.0f;
	float b1 = ((col1) & 0xff) / 255.0f;

	float a2 = ((col2 >> 24) & 0xff) / 255.0f;
	float r2 = ((col2 >> 16) & 0xff) / 255.0f;
	float g2 = ((col2 >> 8) & 0xff) / 255.0f;
	float b2 = ((col2) & 0xff) / 255.0f;
	glDisable2(GL_TEXTURE_2D);
	glEnable2(GL_BLEND);
	glDisable2(GL_ALPHA_TEST);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel2(GL_SMOOTH);

	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(r2, g2, b2, a2);
	t.vertex(x1, y0, 0);
	t.color(r1, g1, b1, a1);
	t.vertex(x0, y0, 0);
	t.color(r1, g1, b1, a1);
	t.vertex(x0, y1, 0);
	t.color(r2, g2, b2, a2);
	t.vertex(x1, y1, 0);
	t.draw();

	glShadeModel2(GL_FLAT);
	glDisable2(GL_BLEND);
	glEnable2(GL_ALPHA_TEST);
	glEnable2(GL_TEXTURE_2D);
}
