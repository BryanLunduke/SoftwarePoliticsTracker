#include "RolledSelectionListV.h"
#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/gles.h"
#include "../../../platform/input/Mouse.h"
#include "../../../util/Mth.h"
#include "../../renderer/Textures.h"


RolledSelectionListV::RolledSelectionListV( Minecraft* minecraft_, int width_, int height_, int x0_, int x1_, int y0_, int y1_, int itemHeight_ )
:	minecraft(minecraft_),
	width(width_),
	height(height_),
	x0((float)x0_),
	x1((float)x1_),
	y0((float)y0_),
	y1((float)y1_),
	itemHeight(itemHeight_),
	selectionY(-1),
	lastSelectionTime(0),
	lastSelection(-1),
	renderSelection(true),
	doRenderHeader(false),
	headerHeight(0),
	dragState(DRAG_OUTSIDE),
	yDrag(0.0f),
	yo(0.0f),
	yoo(0.0f),
	yInertia(0.0f),
	_componentSelected(false),
	_renderDirtBackground(true),
	_renderTopBorder(true),
	_renderBottomBorder(true),
	_lastyoo(0),
	_yinertia(0),
	_stickPixels(0),
	_lastxm(0),
	_lastym(0)
{
	yo = yoo = 0;//(float)(-itemHeight) * 0.5f;
	_lastyoo = yoo;
}

void RolledSelectionListV::setRenderSelection( bool _renderSelection )
{
	renderSelection = _renderSelection;
}

void RolledSelectionListV::setComponentSelected(bool selected) {
	_componentSelected = selected;
}

void RolledSelectionListV::setRenderHeader( bool _renderHeader, int _headerHeight )
{
	doRenderHeader = _renderHeader;
	headerHeight = _headerHeight;

	if (!doRenderHeader) {
		headerHeight = 0;
	}
}

int RolledSelectionListV::getMaxPosition()
{
	return getNumberOfItems() * itemHeight + headerHeight;
}

int RolledSelectionListV::getItemAtPosition( int x, int y )
{
	int clickSlotPos = (int)(y - y0 - headerHeight + (int) yo - 4);
	int isInsideX = x >= x0 && x <= x1;
	return isInsideX? getItemAtYPositionRaw(clickSlotPos) : -1;
}

int RolledSelectionListV::getItemAtYPositionRaw(int y) {
	int slot = y / itemHeight;
	bool isInsideX = slot >= 0 && y >= 0 && slot < getNumberOfItems();
	return isInsideX? slot : -1;
}

bool RolledSelectionListV::capYPosition()
{
	float max = getMaxPosition() - (y1 - y0 - 4);
	if (max < 0) max /= 2;
	if (yo < 0) yo = 0;
	if (yo > max) yo = max;
	return false;
/*
	const float MinY = -itemHeight/2;//(float)(itemHeight-height)/2;
	const float MaxY = MinY + (getNumberOfItems()-1) * itemHeight;
	if (yo < MinY) { yo = MinY; yInertia = 0; return true; }
	if (yo > MaxY) { yo = MaxY; yInertia = 0; return true; }
	return false;
	*/
}

void RolledSelectionListV::tick() {

	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT))
	{
		_yinertia = _lastyoo - yoo;
	}
	_lastyoo = yoo;
	
	//yInertia = Mth::absDecrease(yInertia, 1.0f, 0);

	yoo = yo - yInertia;

	//LOGI("tick: %f, %f, %f\n", yo, yInertia, _yinertia);
}

float RolledSelectionListV::getPos(float alpha) {
	return yoo - yInertia * alpha;
}

void RolledSelectionListV::render( int xm, int ym, float a )
{
	_lastxm = xm;
	_lastym = ym;
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
				lastSelection = convertSelection( getItemAtPosition(width/2, ym), xm, ym );
				selectStart(lastSelection);
				//LOGI("Sel : %d\n", lastSelection);
				selectionY = ym;
				_stickPixels = 10;
			} 
			else if (dragState >= 0) {
				float delta = (ym - yDrag);
				float absDelta = Mth::abs(delta);
				if (absDelta > _stickPixels) {
					_stickPixels = 0;
					delta -= delta>0? _stickPixels : -_stickPixels;
				} else {
					delta = 0;
					_stickPixels -= absDelta;
				}
				yo -= delta;
				yoo = yo;
			}
			dragState = DRAG_NORMAL;
		}
	} else {
		if (dragState >= 0) {
			if (dragState >= 0) {
				yInertia = _yinertia < 0? Mth::Max(-10.0f, _yinertia) : Mth::Min(10.0f, _yinertia);
			}
			// kill small inertia values when releasing scrollist
			if (std::abs(yInertia) <= 2.0001f) {
				yInertia = 0.0f;
			}

			if (std::abs(yInertia) <= 10 /*&& getTimeMs() - lastSelectionTime < 300 */)
			{
				//float clickSlotPos = (ym - x0 - headerHeight + (int) yo - 4);
				int slot = convertSelection( getItemAtPosition(width/2, ym), xm, ym);
				//LOGI("slot: %d, lt: %d. diff: %d - %d\n", slot, lastSelection, selectionX, xm);
				if (xm >= x0 && xm <= x1 && slot >= 0 && slot == lastSelection && std::abs(selectionY - ym) < 10)
					selectItem(slot, false);
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

		yo = getPos(a);
	}
	yDrag = (float)ym;

	evaluate(xm, ym);
	capYPosition();

	Tesselator& t = Tesselator::instance;

	const int HalfWidth = 48;
	int rowX = (int)(width / 2 - HalfWidth + 8);
	int rowBaseY = (int)(y0 + 4 - (int) yo);

	if (_renderDirtBackground)
		renderDirtBackground();

	if (getNumberOfItems() == 0) yo = 0;

	//int rowY = (int)(height / 2 - HalfHeight + 8);
	if (doRenderHeader) {
		const int HalfWidth = 48;
		int rowX = (int)(width / 2 - HalfWidth + 8);
		int rowBaseY = (int)(y0 + 4 - (int) yo);
		renderHeader(rowX, rowBaseY, t);
	}

	onPreRender();
	for (int i = 0; i < itemCount; i++) {

		float y = (float)(rowBaseY + (i) * itemHeight + headerHeight);
		float h = itemHeight - 4.0f;

		if (y > y1 || (y + h) < y0) {
			continue;
		}

		if (renderSelection && isSelectedItem(i)) {
			//float y0 = height / 2.0f - HalfHeight - 4;
			//float y1 = height / 2.0f + HalfHeight - 4;
			//glColor4f2(1, 1, 1, 1);
			//glDisable2(GL_TEXTURE_2D);

			//int ew = 0;
			//int color = 0x808080;
			//if (_componentSelected) {
			//	ew = 0;
			//	color = 0x7F89BF;
			//}
			//t.begin();
			//t.color(color);
			//t.vertex(x - 2 - ew, y0 - ew, 0);
			//t.vertex(x - 2 - ew, y1 + ew, 0);
			//t.vertex(x + h + 2 + ew, y1 + ew, 0);
			//t.vertex(x + h + 2 + ew, y0 - ew, 0);

			//t.color(0x000000);
			//t.vertex(x - 1, y0 + 1,  0);
			//t.vertex(x - 1, y1 - 1,  0);
			//t.vertex(x + h + 1, y1 - 1, 0);
			//t.vertex(x + h + 1, y0 + 1, 0);

			//t.draw();
			//glEnable2(GL_TEXTURE_2D);
		}
		renderItem(i, rowX, (int)y, (int)h, t);
	}
	onPostRender();

	glDisable2(GL_DEPTH_TEST);

	if (_renderTopBorder)
		renderHoleBackground(0, y0, 255, 255);
	if (_renderBottomBorder)
		renderHoleBackground(y1, (float)height, 255, 255);
	renderForeground();

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
	//glEnable2(GL_DEPTH_TEST);

	//glShadeModel2(GL_FLAT);
	//glEnable2(GL_ALPHA_TEST);
	//glDisable2(GL_BLEND);
}

void RolledSelectionListV::renderHoleBackground( /*float x0, float x1,*/ float y0, float y1, int a0, int a1 )
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

void RolledSelectionListV::touched()
{
}

void RolledSelectionListV::evaluate(int xm, int ym)
{
	if (std::abs(selectionY - ym) >= 10) {
		lastSelection = -1;
		selectCancel();
	}
}

void RolledSelectionListV::onPreRender()
{
}

void RolledSelectionListV::onPostRender()
{
}

void RolledSelectionListV::renderDirtBackground()
{
	float by0 = _renderTopBorder?    y0 : 0;
	float by1 = _renderBottomBorder? y1 : height;

	minecraft->textures->loadAndBindTexture("gui/background.png");
	glColor4f2(1.0f, 1, 1, 1);
	float s = 32;
	const float uvy = (float)((int) yo);
	Tesselator& t = Tesselator::instance;
	t.begin();
	t.color(0x202020);
	t.vertexUV(x0, by1, 0, x0 / s, (by1+uvy) / s);
	t.vertexUV(x1, by1, 0, x1 / s, (by1+uvy) / s);
	t.vertexUV(x1, by0, 0, x1 / s, (by0+uvy) / s);
	t.vertexUV(x0, by0, 0, x0 / s, (by0+uvy) / s);
	t.draw();
	//LOGI("%f, %f - %f, %f\n", x0, by0, x1, by1);
}
