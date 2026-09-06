#include "RolledSelectionListH.h"
#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../../platform/input/Multitouch.h"
#include "../../../util/Mth.h"
#include "../../renderer/Textures.h"


RolledSelectionListH::RolledSelectionListH( Minecraft* minecraft, int width, int height, int x0, int x1, int y0, int y1, int itemWidth )
:	minecraft(minecraft),
	width(width),
	height(height),
	x0((float)x0),
	x1((float)x1),
	y0((float)y0),
	y1((float)y1),
	itemWidth(itemWidth),
	selectionX(-1),
	lastSelectionTime(0),
	lastSelection(-1),
	renderSelection(true),
	doRenderHeader(false),
	headerWidth(0),
	dragState(DRAG_OUTSIDE),
	xDrag(0.0f),
	xo(0.0f),
	xoo(0.0f),
	xInertia(0.0f),
	_componentSelected(false),
	_renderTopBorder(true),
	_renderBottomBorder(true),
	_lastxoo(0),
	_xinertia(0)
{
	xo = xoo = (float)(itemWidth-width) * 0.5f;
	_lastxoo = xoo;
}

void RolledSelectionListH::setRenderSelection( bool _renderSelection )
{
	renderSelection = _renderSelection;
}

void RolledSelectionListH::setComponentSelected(bool selected) {
	_componentSelected = selected;
}

void RolledSelectionListH::setRenderHeader( bool _renderHeader, int _headerHeight )
{
	doRenderHeader = _renderHeader;
	headerWidth = _headerHeight;

	if (!doRenderHeader) {
		headerWidth = 0;
	}
}

int RolledSelectionListH::getMaxPosition()
{
	return getNumberOfItems() * itemWidth + headerWidth;
}

int RolledSelectionListH::getItemAtPosition( int x, int y )
{
	int clickSlotPos = (int)(x - x0 - headerWidth + (int) xo - 4);
	int isInsideY = y >= y0 && y <= y1;
	return isInsideY? getItemAtXPositionRaw(clickSlotPos) : -1;
}

int RolledSelectionListH::getItemAtXPositionRaw(int x) {
	int slot = x / itemWidth;
	bool isInsideX = slot >= 0 && x >= 0 && slot < getNumberOfItems();
	return isInsideX? slot : -1;
}

bool RolledSelectionListH::capXPosition()
{
	const float MinX = (float)(itemWidth-width)/2;
	const float MaxX = MinX + (getNumberOfItems()-1) * itemWidth;
	if (xo < MinX) { xo = MinX; xInertia = 0; return true; }
	if (xo > MaxX) { xo = MaxX; xInertia = 0; return true; }
	return false;
}

void RolledSelectionListH::tick() {

	//if (Mouse::isButtonDown(MouseAction::ACTION_LEFT))
	{
		_xinertia = _lastxoo - xoo;
	}
	_lastxoo = xoo;
	xoo = xo - xInertia;
}

float RolledSelectionListH::getPos(float alpha) {
	return xoo - xInertia * alpha;
}

void RolledSelectionListH::render( int xm, int ym, float a )
{
	renderBackground();

	int itemCount = getNumberOfItems();

	//float yy0 = height / 2.0f + 124;
	//float yy1 = yy0 + 6;

	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		touched();
		//LOGI("DOWN ym: %d\n", ym);
		if (ym >= y0 && ym <= y1) {
			if (dragState == NO_DRAG) {
				lastSelectionTime = getTimeMs();
				lastSelection = getItemAtPosition(xm, height/2);
				//float localX = (float)(xm*Gui::InvGuiScale - x0 - xo + lastSelection * itemWidth + headerWidth);
				selectStart(lastSelection, 0, 0);//localX, ym-y0);
				selectionX = xm;
			} 
			else if (dragState >= 0) {
				xo -= (xm - xDrag);
				xoo = xo;
			}
			dragState = DRAG_NORMAL;
			//const int* ids;
			//LOGI("mtouch: %d\n", Multitouch::getActivePointerIds(&ids));
		}
	} else {
		if (dragState >= 0) {
			if (dragState >= 0) {
				xInertia = _xinertia < 0? Mth::Max(-20.0f, _xinertia) : Mth::Min(20.0f, _xinertia);
			}
			//LOGI("Inertia: %f. Time: %d, delta-x: %d, (xm, sel: %d, %d)\n", xInertia, getTimeMs() - lastSelectionTime, std::abs(selectionX - xm), xm, selectionX);
			// kill small inertia values when releasing scrollist
			if (std::abs(xInertia) <= 2.0001f) {
				xInertia = 0.0f;
			}

			if (std::abs(xInertia) <= 10 && getTimeMs() - lastSelectionTime < 300)
			{
				int slot = getItemAtPosition(xm, height/2);
				//LOGI("slot: %d, lt: %d. diff: %d - %d\n", slot, lastSelection, selectionX, xm);
				if (slot >= 0 && slot == lastSelection && std::abs(selectionX - xm) < 10)
					selectItem(slot, false);
				else
					selectCancel();
			} else {
				selectCancel();
			}
		}

		//	if (slot >= 0 && std::abs(selectionX - xm) < itemWidth)
		//	{
		//		bool doubleClick = false;
		//		selectItem(slot, doubleClick);
		//		//xInertia = 0.0f;
		//	}
		//}
		dragState = NO_DRAG;

		xo = getPos(a);
	}
	xDrag = (float)xm;

	capXPosition();

	Tesselator& t = Tesselator::instance;

	float by0 = _renderTopBorder?    y0 : 0;
	float by1 = _renderBottomBorder? y1 : height;

	//LOGI("x: %f\n", xo);

	minecraft->textures->loadAndBindTexture("gui/background.png");
	glColor4f2(1.0f, 1, 1, 1);
	float s = 32;
	t.begin();
	t.color(0x202020);
	t.vertexUV(x0, by1, 0, (x0 + (int) xo) / s, by1 / s);
	t.vertexUV(x1, by1, 0, (x1 + (int) xo) / s, by1 / s);
	t.vertexUV(x1, by0, 0, (x1 + (int) xo) / s, by0 / s);
	t.vertexUV(x0, by0, 0, (x0 + (int) xo) / s, by0 / s);
	t.draw();

	const int HalfHeight = 48;

	if (getNumberOfItems() == 0) xo = 0;

	int rowY = (int)(height / 2 - HalfHeight + 8);
	int rowBaseX = (int)(x0 /*+ 4*/ - (int) xo);

	if (doRenderHeader) {
		renderHeader(rowBaseX, rowY, t);
	}

	for (int i = 0; i < itemCount; i++) {

		float x = (float)(rowBaseX + (i) * itemWidth + headerWidth);
		float h = (float)itemWidth;

		if (x > x1 || (x + h) < x0) {
			continue;
		}

		if (renderSelection && isSelectedItem(i)) {
			float y0 = height / 2.0f - HalfHeight - 4; //@kindle-res:+2
			float y1 = height / 2.0f + HalfHeight - 4; //@kindle-res:-6
			glColor4f2(1, 1, 1, 1);
			glDisable2(GL_TEXTURE_2D);

			int ew = 0;
			int color = 0x808080;
			if (_componentSelected) {
				ew = 0;
				color = 0x7F89BF;
			}
			t.begin();
			t.color(color);
			t.vertex(x - 1 - ew, y0 - ew, 0);
			t.vertex(x - 1 - ew, y1 + ew, 0);
			t.vertex(x + h + 1 + ew, y1 + ew, 0);
			t.vertex(x + h + 1 + ew, y0 - ew, 0);

			t.color(0x000000);
			t.vertex(x, y0 + 1,  0);
			t.vertex(x, y1 - 1,  0);
			t.vertex(x + h, y1 - 1, 0);
			t.vertex(x + h, y0 + 1, 0);

			t.draw();
			glEnable2(GL_TEXTURE_2D);
		}
		renderItem(i, (int)x, rowY, (int)h, t);
	}

	glDisable2(GL_DEPTH_TEST);

	if (_renderTopBorder)
		renderHoleBackground(0, y0, 255, 255);
	if (_renderBottomBorder)
		renderHoleBackground(y1, (float)height, 255, 255);

	//glEnable2(GL_BLEND);
	//glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glDisable2(GL_ALPHA_TEST);
	//glShadeModel2(GL_SMOOTH);

	//glDisable2(GL_TEXTURE_2D);

	//const int d = 4;
	//t.begin();
	//t.color(0x000000, 0);
	//t.vertexUV(y0, x0 + d, 0, 0, 1);
	//t.vertexUV(y1, x0 + d, 0, 1, 1);
	//t.color(0x000000, 255);
	//t.vertexUV(y1, x0, 0, 1, 0);
	//t.vertexUV(y0, x0, 0, 0, 0);
	//t.draw();

	//t.begin();
	//t.color(0x000000, 255);
	//t.vertexUV(y0, x1, 0, 0, 1);
	//t.vertexUV(y1, x1, 0, 1, 1);
	//t.color(0x000000, 0);
	//t.vertexUV(y1, x1 - d, 0, 1, 0);
	//t.vertexUV(y0, x1 - d, 0, 0, 0);
	//t.draw();

	//renderDecorations(xm, ym);

	//glEnable2(GL_TEXTURE_2D);
	glEnable2(GL_DEPTH_TEST);

	//glShadeModel2(GL_FLAT);
	//glEnable2(GL_ALPHA_TEST);
	//glDisable2(GL_BLEND);
}

void RolledSelectionListH::renderHoleBackground( /*float x0, float x1,*/ float y0, float y1, int a0, int a1 )
{
	Tesselator& t = Tesselator::instance;
	minecraft->textures->loadAndBindTexture("gui/background.png");
	glColor4f2(1.0f, 1, 1, 1);
	float s = 32;
	t.begin();
	t.color(0x505050, a1);
	t.vertexUV(0, y1, 0, 0, y1 / s);
	t.vertexUV((float)width, y1, 0, width / s, y1 / s);
	t.color(0x505050, a0);
	t.vertexUV((float)width, y0, 0, width / s, y0 / s);
	t.vertexUV(0, y0, 0, 0, y0 / s);
	t.draw();
	//printf("x, y, x1, y1: %d, %d, %d, %d\n", 0, (int)y0, width, (int)y1);
}

void RolledSelectionListH::touched()
{
}
