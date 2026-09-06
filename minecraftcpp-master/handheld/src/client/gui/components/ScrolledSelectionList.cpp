#include "ScrolledSelectionList.h"
#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../renderer/Textures.h"

static int Abs(int d) {
	return d >= 0? d : -d;
}

ScrolledSelectionList::ScrolledSelectionList( Minecraft* _minecraft, int _width, int _height, int _y0, int _y1, int _itemHeight )
:	minecraft(_minecraft),
	width(_width),
	height(_height),
	y0((float)_y0),
	y1((float)_y1),
	itemHeight(_itemHeight),
	x0(0.0f),
	x1((float)_width),
	selectionY(-1),
	lastSelectionTime(0),
	renderSelection(true),
	doRenderHeader(false),
	headerHeight(0),
	dragState(DRAG_OUTSIDE),
	yDrag(0.0f),
	yo(0.0f),
	yInertia(0.0f)
{
}

void ScrolledSelectionList::setRenderSelection( bool _renderSelection )
{
	renderSelection = _renderSelection;
}

void ScrolledSelectionList::setRenderHeader( bool _renderHeader, int _headerHeight )
{
	doRenderHeader = _renderHeader;
	headerHeight = _headerHeight;

	if (!doRenderHeader) {
		headerHeight = 0;
	}
}

int ScrolledSelectionList::getMaxPosition()
{
	return getNumberOfItems() * itemHeight + headerHeight;
}

int ScrolledSelectionList::getItemAtPosition( int x, int y )
{
	int x0 = width / 2 - (92 + 16 + 2);
	int x1 = width / 2 + (92 + 16 + 2);

	int clickSlotPos = (int)(y - y0 - headerHeight + (int) yo - 4);
	int slot = clickSlotPos / itemHeight;
	if (x >= x0 && x <= x1 && slot >= 0 && clickSlotPos >= 0 && slot < getNumberOfItems()) {
		return slot;
	}
	return -1;
}

void ScrolledSelectionList::capYPosition()
{
	float max = getMaxPosition() - (y1 - y0 - 4);
	if (max < 0) max /= 2;
	if (yo < 0) yo = 0;
	if (yo > max) yo = max;
}

void ScrolledSelectionList::render( int xm, int ym, float a )
{
	renderBackground();

	int itemCount = getNumberOfItems();

	//float xx0 = width / 2.0f + 124;
	//float xx1 = xx0 + 6;


	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT)) {
		//LOGI("DOWN ym: %d\n", ym);
		if (ym >= y0 && ym <= y1 && ym != ignoreY) {
			if (dragState == NO_DRAG) {
				dragState = DRAG_SKIP;
			} 
			else if (dragState >= 0)
			{
				if (dragState == DRAG_SKIP)
				{
					lastSelectionTime = getTimeMs();
					selectionY = ym;
				}
				else if (dragState == DRAG_NORMAL)
				{
					yo -= (ym - yDrag);
					yInertia += (float)(ym - yDrag);
				}
				dragState = DRAG_NORMAL;
			}
			ignoreY = -1;
		}

	} else {
		if (dragState != NO_DRAG)
		{
			//LOGI("UP ym: %d\n", ym);
		}
		//ignoreY = ym;

		// kill small inertia values when releasing scrollist
		if (dragState >= 0 && std::abs(yInertia) < 2)
		{
			yInertia = 0.0f;
		}

		if (dragState >= 0 && getTimeMs() - lastSelectionTime < 300)
		{
			float clickSlotPos = (ym - y0 - headerHeight + (int) yo - 4);
			int slot = (int)clickSlotPos / itemHeight;

			if (slot >= 0 && Abs(selectionY - ym) < itemHeight)
			{
				bool doubleClick = false;
				selectItem(slot, doubleClick);
				yInertia = 0.0f;
			}
		}
		dragState = NO_DRAG;

		yo -= yInertia;
	}
	yInertia = yInertia * .75f;
	yDrag = (float)ym;

	capYPosition();

	Tesselator& t = Tesselator::instance;

	renderDirtBackground();

	int rowX = (int)(width / 2 - 92 - 16);
	int rowBaseY = (int)(y0 + 4 - (int) yo);

	if (doRenderHeader) {
		renderHeader(rowX, rowBaseY, t);
	}

	for (int i = 0; i < itemCount; i++) {

		float y = (float)(rowBaseY + (i) * itemHeight + headerHeight);
		float h = itemHeight - 4.0f;

		if (y > y1 || (y + h) < y0) {
			continue;
		}

		if (renderSelection && isSelectedItem(i)) {
			float x0 = width / 2.0f - (92 + 16 + 2);
			float x1 = width / 2.0f + (92 + 16 + 2);
			glColor4f2(1, 1, 1, 1);
			glDisable2(GL_TEXTURE_2D);
			t.begin();
			t.color(0x808080);
			t.vertexUV(x0, y + h + 2, 0, 0, 1);
			t.vertexUV(x1, y + h + 2, 0, 1, 1);
			t.vertexUV(x1, y - 2, 0, 1, 0);
			t.vertexUV(x0, y - 2, 0, 0, 0);

			t.color(0x000000);
			t.vertexUV(x0 + 1, y + h + 1, 0, 0, 1);
			t.vertexUV(x1 - 1, y + h + 1, 0, 1, 1);
			t.vertexUV(x1 - 1, y - 1, 0, 1, 0);
			t.vertexUV(x0 + 1, y - 1, 0, 0, 0);

			t.draw();
			glEnable2(GL_TEXTURE_2D);
		}

		renderItem(i, rowX, (int)y, (int)h, t);

	}

	glDisable2(GL_DEPTH_TEST);


	int d = 4;

	renderHoleBackground(0, y0, 255, 255);
	renderHoleBackground(y1, (float)height, 255, 255);

	glEnable2(GL_BLEND);
	glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable2(GL_ALPHA_TEST);
	glShadeModel2(GL_SMOOTH);

	glDisable2(GL_TEXTURE_2D);

	t.begin();
	t.color(0x000000, 0);
	t.vertexUV(x0, y0 + d, 0, 0, 1);
	t.vertexUV(x1, y0 + d, 0, 1, 1);
	t.color(0x000000, 255);
	t.vertexUV(x1, y0, 0, 1, 0);
	t.vertexUV(x0, y0, 0, 0, 0);
	t.draw();

	t.begin();
	t.color(0x000000, 255);
	t.vertexUV(x0, y1, 0, 0, 1);
	t.vertexUV(x1, y1, 0, 1, 1);
	t.color(0x000000, 0);
	t.vertexUV(x1, y1 - d, 0, 1, 0);
	t.vertexUV(x0, y1 - d, 0, 0, 0);
	t.draw();

	// 		{
	// 			float max = getMaxPosition() - (y1 - y0 - 4);
	// 			if (max > 0) {
	// 				float barHeight = (y1 - y0) * (y1 - y0) / (getMaxPosition());
	// 				if (barHeight < 32) barHeight = 32;
	// 				if (barHeight > (y1 - y0 - 8)) barHeight = (y1 - y0 - 8);
	// 
	// 				float yp = (int) yo * (y1 - y0 - barHeight) / max + y0;
	// 				if (yp < y0) yp = y0;
	// 
	// 				t.begin();
	// 				t.color(0x000000, 255);
	// 				t.vertexUV(xx0, y1, 0.0f, 0.0f, 1.0f);
	// 				t.vertexUV(xx1, y1, 0.0f, 1.0f, 1.0f);
	// 				t.vertexUV(xx1, y0, 0.0f, 1.0f, 0.0f);
	// 				t.vertexUV(xx0, y0, 0.0f, 0.0f, 0.0f);
	// 				t.draw();
	// 
	// 				t.begin();
	// 				t.color(0x808080, 255);
	// 				t.vertexUV(xx0, yp + barHeight, 0, 0, 1);
	// 				t.vertexUV(xx1, yp + barHeight, 0, 1, 1);
	// 				t.vertexUV(xx1, yp, 0, 1, 0);
	// 				t.vertexUV(xx0, yp, 0, 0, 0);
	// 				t.draw();
	// 
	// 				t.begin();
	// 				t.color(0xc0c0c0, 255);
	// 				t.vertexUV(xx0, yp + barHeight - 1, 0, 0, 1);
	// 				t.vertexUV(xx1 - 1, yp + barHeight - 1, 0, 1, 1);
	// 				t.vertexUV(xx1 - 1, yp, 0, 1, 0);
	// 				t.vertexUV(xx0, yp, 0, 0, 0);
	// 				t.draw();
	// 			}
	// 		}

	renderDecorations(xm, ym);


	glEnable2(GL_TEXTURE_2D);
	glEnable2(GL_DEPTH_TEST);

	glShadeModel2(GL_FLAT);
	glEnable2(GL_ALPHA_TEST);
	glDisable2(GL_BLEND);
}

void ScrolledSelectionList::renderHoleBackground( float y0, float y1, int a0, int a1 )
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
}

void ScrolledSelectionList::renderDirtBackground()
{
	Tesselator& t = Tesselator::instance;
	minecraft->textures->loadAndBindTexture("gui/background.png");
	glColor4f2(1.0f, 1, 1, 1);
	float s = 32;
	t.begin();
	t.color(0x202020);
	t.vertexUV(x0, y1, 0, x0 / s, (y1 + (int) yo) / s);
	t.vertexUV(x1, y1, 0, x1 / s, (y1 + (int) yo) / s);
	t.vertexUV(x1, y0, 0, x1 / s, (y0 + (int) yo) / s);
	t.vertexUV(x0, y0, 0, x0 / s, (y0 + (int) yo) / s);
	t.draw();
}
