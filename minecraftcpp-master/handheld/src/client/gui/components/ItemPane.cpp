#include "ItemPane.h"
#include "../Gui.h"
#include "../../renderer/gles.h"
#include "../../renderer/Tesselator.h"
#include "NinePatch.h"
#include "../../renderer/entity/ItemRenderer.h"

const int rgbActive = 0xfff0f0f0;
const int rgbInactive = 0xc0635558;
const int rgbInactiveShadow = 0xc0aaaaaa;

ItemPane::ItemPane( IItemPaneCallback* screen,
					Textures* textures,
					const IntRectangle& rect,
					int numItems,
					int guiHeight,
					int physicalScreenHeight,
					bool isVertical /*= true*/)
:	super(
	(isVertical?SF_LockX:SF_LockY)/*|SF_Scissor*/|SF_ShowScrollbar,
	rect,							// Pane rect
	isVertical?IntRectangle(0, 0, rect.w, 22)  // Item rect if vertical
			  :IntRectangle(0, 0, 32, rect.h), // Item rect if horizontal
	isVertical?1:numItems, numItems, Gui::GuiScale),
	screen(screen),
	textures(textures),
	physicalScreenHeight(physicalScreenHeight),
	guiSlotItem(NULL),
	guiSlotItemSelected(NULL),
	isVertical(isVertical)
{
	// Expand the area to make it easier to scroll
	area._x0 -= 4;
	area._x1 += 4;
	area._y0 = 0;
	area._y1 = (float)guiHeight;

	// GUI
	NinePatchFactory builder(textures, "gui/spritesheet.png");
	guiSlotItem = builder.createSymmetrical(IntRectangle(20, 32, 8, 8), 2, 2);
	guiSlotItemSelected = builder.createSymmetrical(IntRectangle(28, 32, 8, 8), 2, 2);
	guiSlotItem->setSize((float)rect.w + 4, 22);
	guiSlotItemSelected->setSize((float)rect.w + 4, 22);
}

ItemPane::~ItemPane() {
	delete guiSlotItem;
	delete guiSlotItemSelected;
}

void ItemPane::renderBatch( std::vector<GridItem>& items, float alpha )
{
	//fill(bbox.x, bbox.y, bbox.x + bbox.w, bbox.y + bbox.h, 0xff666666);
	const std::vector<CItem*>& cat = screen->getItems(this);
	if (cat.empty()) return;

	glEnable2(GL_SCISSOR_TEST);
	GLuint x = (GLuint)(screenScale * bbox.x);
	GLuint y = physicalScreenHeight - (GLuint)(screenScale * (bbox.y + bbox.h));
	GLuint w = (GLuint)(screenScale * bbox.w);
	GLuint h = (GLuint)(screenScale * bbox.h);
	glScissor(x, y, w, h);

	Tesselator& t = Tesselator::instance;

	t.beginOverride();
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		(item.selected? guiSlotItemSelected : guiSlotItem)->draw(t, Gui::floorAlignToScreenPixel(item.xf-1), Gui::floorAlignToScreenPixel(item.yf));
	}
	t.endOverrideAndDraw();

	t.beginOverride();
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		CItem* citem = cat[item.id];

		ItemRenderer::renderGuiItem(NULL, textures, &citem->item,
			Gui::floorAlignToScreenPixel(item.xf + itemBbox.w - 16),
			Gui::floorAlignToScreenPixel(2 + item.yf), 16, 16, false);
	}
	t.endOverrideAndDraw();

	t.beginOverride();
	for (unsigned int i = 0; i < items.size(); ++i) {
		GridItem& item = items[i];
		CItem* citem = cat[item.id];

		char buf[64] = {0};
		int c = Gui::itemCountItoa(buf, citem->inventoryCount);

		float xf = item.xf - 1;
		if (citem->canCraft()) {
			f->drawShadow(citem->text,
				Gui::floorAlignToScreenPixel(xf + 2),
				Gui::floorAlignToScreenPixel(item.yf + 6), rgbActive);
			t.scale2d(0.6667f, 0.6667f);
			f->drawShadow(buf,
				Gui::floorAlignToScreenPixel(1.5f * (xf + itemBbox.w - c*4)),
				Gui::floorAlignToScreenPixel(1.5f * (item.yf + itemBbox.h - 8)), rgbActive);
			t.resetScale();
		} else {
			f->draw(citem->text,
				Gui::floorAlignToScreenPixel(xf + 3),
				Gui::floorAlignToScreenPixel(item.yf + 7), rgbInactiveShadow);
			f->draw(citem->text,
				Gui::floorAlignToScreenPixel(xf + 2),
				Gui::floorAlignToScreenPixel(item.yf + 6), rgbInactive);
			t.scale2d(0.6667f, 0.6667f);
			f->draw(buf,
				Gui::floorAlignToScreenPixel(1.5f * (xf + itemBbox.w - c*4)),
				Gui::floorAlignToScreenPixel(1.5f * (item.yf + itemBbox.h - 8)), rgbInactive);
			t.resetScale();
		}
	}
	t.endOverrideAndDraw();

	//fillGradient(bbox.x, bbox.y, bbox.x + bbox.w, 20, 0x00000000, 0x80ff0000)
	if (isVertical) {
		fillGradient(bbox.x, bbox.y, bbox.x + bbox.w, bbox.y + 28, 0xbb000000, 0x00000000);
		fillGradient(bbox.x, bbox.y + bbox.h - 28, bbox.x + bbox.w, bbox.y + bbox.h, 0x00000000, 0xbb000000);//0xbb2A272B);
	} else {
		fillHorizontalGradient(bbox.x, bbox.y, bbox.x + 28, bbox.y + bbox.h, 0xbb000000, 0x00000000);
		fillHorizontalGradient(bbox.x + bbox.w - 28, bbox.y, bbox.x + bbox.w, bbox.y + bbox.h, 0x00000000, 0xbb000000);//0xbb2A272B);
	}

	//LOGI("scroll: %f - %f, %f :: %f, %f\n", hScroll.alpha, hScroll.x, hScroll.y, hScroll.w, hScroll.h);
	glDisable2(GL_SCISSOR_TEST);

	drawScrollBar(hScroll);
	drawScrollBar(vScroll);
}

bool ItemPane::onSelect( int gridId, bool selected )
{
	if (selected)
		screen->onItemSelected(this, gridId);

	return selected;
}

void ItemPane::drawScrollBar( ScrollBar& sb ) {
	if (sb.alpha <= 0)
		return;

	int color = ((int)(255.0f * sb.alpha) << 24) | 0xffffff;
	fill(2 + sb.x, sb.y, 2 + sb.x + sb.w, sb.y + sb.h, color);
}
