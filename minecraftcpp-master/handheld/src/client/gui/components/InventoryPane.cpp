#include "InventoryPane.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../player/input/touchscreen/TouchAreaModel.h"
#include "../../renderer/entity/ItemRenderer.h"
#include "../../renderer/Tesselator.h"
#include "../../renderer/Textures.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/entity/player/Inventory.h"

namespace Touch {

static const int By = 6; // Border Frame height

InventoryPane::InventoryPane( IInventoryPaneCallback* screen, Minecraft* mc, const IntRectangle& rect, int paneWidth, float clickMarginH, int numItems, int itemSize, int itemBorderSize)
:	screen(screen),
	mc(mc),
	paneWidth(paneWidth),
	rect(rect),
	super(
	SF_LockX|/*SF_Scissor|*/SF_ShowScrollbar|SF_NoHoldSelect,
	rect,  // Pane rect
	IntRectangle(0, 0, itemSize, itemSize), // Item rect
	0, numItems, Gui::GuiScale),
    BorderPixels(itemBorderSize),
	lastItemIndex(-1),
	lastItemTicks(-1),
	fillMarginX(2),
	fillMarginY(4),
	markerType(1),
	markerIndex(-1),
	markerShare(0),
	renderDecorations(true)
{
	_clickArea = new RectangleArea(0, 0, 0, 0);
	area._x0 = rect.x - clickMarginH;
	area._x1 = rect.x + rect.w + clickMarginH;
	area._y0 -= By;
	area._y1 += By;

	/*
	const int left = bbox.x + (bbox.w - paneWidth) / 2;
	bg.x = left;
	bg.w = left + paneWidth; // @note: read as x1, not width
	bg.y = bbox.y - fillMarginY;
	bg.h = bbox.y + bbox.h + fillMarginY; // @note: read as y1, not width
	*/
}

InventoryPane::~InventoryPane() {
	delete _clickArea;
}

void InventoryPane::renderBatch( std::vector<GridItem>& items, float alpha )
{
	//fill(bg.x, bg.y, bg.w, bg.h, 0xff333333);
	fill((float)(bbox.x-fillMarginX-1), (float)(bbox.y-fillMarginY), (float)(bbox.x + bbox.w + fillMarginX+1), (float)(bbox.y + bbox.h + fillMarginY), 0xff333333);
	//fill(0.0f, (float)(bbox.y-fillMarginY), 400.0f, (float)(bbox.y + bbox.h + fillMarginY), 0xff333333);//(float)(bbox.x-fillMarginX), (float)(bbox.y-fillMarginY), (float)(bbox.x + bbox.w + fillMarginX), (float)(bbox.y + bbox.h + fillMarginY), 0xff333333);
	glEnable2(GL_BLEND);
	glDisable2(GL_ALPHA_TEST);
	std::vector<const ItemInstance*> inventoryItems = screen->getItems(this);

	glEnable2(GL_SCISSOR_TEST);
	GLuint x = (GLuint)(screenScale * bbox.x);
	GLuint y = mc->height - (GLuint)(screenScale * (bbox.y + bbox.h));
	GLuint w = (GLuint)(screenScale * bbox.w);
	GLuint h = (GLuint)(screenScale * bbox.h);
	glScissor(x, y, w, h);

	Tesselator& t = Tesselator::instance;

	t.beginOverride();
	t.colorABGR(0xffffffff);
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		blit(item.xf, item.yf, 200, 46, (float)itemBbox.w, (float)itemBbox.h, 16, 16);
	}
	mc->textures->loadAndBindTexture("gui/gui.png");
	t.endOverrideAndDraw();

	GridItem* marked = NULL;
	float mxx, myy;

	t.beginOverride();
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		int j = item.id;
		const ItemInstance* citem = inventoryItems[j];
		if (!citem) continue;

		bool allowed = true;

		t.enableColor();
		//#ifdef DEMO_MODE //@huge @attn
		if (!screen->isAllowed(j)) { allowed = false; t.color( 64,  64,  64); }
		else
			//#endif
			if (lastItemTicks > 0 && lastItemIndex == j) {
				int gv = 255 - lastItemTicks * 15;
				t.color(gv, gv, gv, (allowed && citem->count <= 0)?0x60:0xff);
			} else {
				t.color(255, 255, 255, (allowed && citem->count <= 0)?0x60:0xff);
			}          
			t.noColor();
			float xx = Gui::floorAlignToScreenPixel(item.xf + BorderPixels + 4);
			float yy = Gui::floorAlignToScreenPixel(item.yf + BorderPixels + 4);
			ItemRenderer::renderGuiItem(NULL, mc->textures, citem, xx, yy, 16, 16, false);

			if (j == markerIndex && markerShare >= 0)
				marked = &item, mxx = xx, myy = yy;

	}
	t.endOverrideAndDraw();

	if (marked) {
		glDisable2(GL_TEXTURE_2D);
		const float yy0 = myy - 5.0f;
		const float yy1 = yy0 + 2;
		fill(mxx, yy0, mxx + 16.0f, yy1, 0xff606060);
		fill(mxx, yy0, mxx + markerShare * 16.0f, yy1, markerType==1?0xff00ff00:0xff476543);
		glEnable2(GL_BLEND);
		glEnable2(GL_TEXTURE_2D);
	}


	if (!mc->isCreativeMode()) {
		const float ikText = Gui::InvGuiScale + Gui::InvGuiScale;
		const float kText = 0.5f * Gui::GuiScale;
		t.beginOverride();
		t.scale2d(ikText, ikText);
		for (unsigned int i = 0; i < items.size(); ++i) {
			GridItem& item = items[i];
			const ItemInstance* citem = inventoryItems[item.id];
			if (!citem) continue;

			char buf[64] = {0};
			/*int c = */ Gui::itemCountItoa(buf, citem->count);

			float tx = Gui::floorAlignToScreenPixel(kText * (item.xf + BorderPixels + 3));
			float ty = Gui::floorAlignToScreenPixel(kText * (item.yf + BorderPixels + 3));
			mc->gui.renderSlotText(citem, tx, ty, true, true);
		}
		t.resetScale();
		glEnable2(GL_BLEND);
		t.endOverrideAndDraw();
	}

	if (renderDecorations) {
		t.beginOverride();
		for (unsigned int i = 0; i < items.size(); ++i) {
			GridItem& item = items[i];
			const ItemInstance* citem = inventoryItems[item.id];
			if (!citem || citem->isNull()) continue;

			if (citem->isDamaged()) {
				ItemRenderer::renderGuiItemDecorations(citem, item.xf + 8, item.yf + 12);
			}
		}

		glDisable2(GL_TEXTURE_2D);
		t.endOverrideAndDraw();
		glEnable2(GL_TEXTURE_2D);
	}
	glDisable2(GL_SCISSOR_TEST);

	//fillGradient(bbox.x - 1, bbox.y, bbox.x + bbox.w + 1, bbox.y + 20, 0x99000000, 0x00000000);
	//fillGradient(bbox.x - 1, bbox.y + bbox.h - 20, bbox.x + bbox.w + 1, bbox.y + bbox.h, 0x00000000, 0x99000000);
	fillGradient(bg.x - fillMarginX, bbox.y, bg.w + fillMarginX, bbox.y + 20, 0x99000000, 0x00000000);
	fillGradient(bg.x - fillMarginX, bbox.y + bbox.h - 20, bg.w + fillMarginX, bbox.y + bbox.h, 0x00000000, 0x99000000);

	drawScrollBar(hScroll);
	drawScrollBar(vScroll);
}

bool InventoryPane::onSelect( int gridId, bool selected )
{
	//screen->onItemSelected(gridId);
	if (screen->isAllowed(gridId))
		if (screen->addItem(this, gridId)) {
			lastItemIndex = gridId;
			lastItemTicks = 7;
		}

		return false;
}

void InventoryPane::drawScrollBar( ScrollBar& sb ) {
	if (sb.alpha <= 0)
		return;

	const int color = ((int)(255.0f * sb.alpha) << 24) | 0xaaaaaa;
	const float xx = (float)(bbox.x + bbox.w);
	fill(xx - sb.w, sb.y, xx, sb.y + sb.h, color);
}

void InventoryPane::tick()
{
	--lastItemTicks;
	super::tick();
}

void InventoryPane::setRenderDecorations( bool value ) {
	renderDecorations = value;
}

}
