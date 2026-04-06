#include "LargeImageButton.h"
#include "../../renderer/Tesselator.h"
#include "../../Minecraft.h"
#include "../../../util/Mth.h"
#include "../../../platform/log.h"
#include "../../../util/Mth.h"
#include "../../renderer/Textures.h"


LargeImageButton::LargeImageButton(int id, const std::string& msg)
:	super(id, msg)
{
	setupDefault();
}

LargeImageButton::LargeImageButton(int id, const std::string& msg, ImageDef& imagedef)
:	super(id, msg)
{
	_imageDef = imagedef;
	setupDefault();
}

void LargeImageButton::setupDefault() {
	_buttonScale = 1;
	width = 72;
	height = 72;
}

void LargeImageButton::render(Minecraft* minecraft, int xm, int ym) {
	if (!visible) return;

	Font* font = minecraft->font;

	//minecraft->textures->loadAndBindTexture("gui/gui.png");
	glColor4f2(1, 1, 1, 1);
	bool hovered = active && (minecraft->useTouchscreen()? (_currentlyDown && xm >= x && ym >= y && xm < x + width && ym < y + height) : false);

	//printf("ButtonId: %d - Hovered? %d (cause: %d, %d, %d, %d, <> %d, %d)\n", id, hovered, x, y, x+w, y+h, xm, ym);
	//int yImage = getYImage(hovered || selected);

	//blit(x, y, 0, 46 + yImage * 20, w / 2, h, 0, 20);
	//blit(x + w / 2, y, 200 - w / 2, 46 + yImage * 20, w / 2, h, 0, 20);

	renderBg(minecraft, xm, ym);

	TextureId texId = (_imageDef.name.length() > 0)? minecraft->textures->loadAndBindTexture(_imageDef.name) : Textures::InvalidId;
	if ( Textures::isTextureIdValid(texId) ) {
		const ImageDef& d = _imageDef;
		Tesselator& t = Tesselator::instance;
		
		t.begin();
			if (!active)				t.color(0xff808080);
			//else if (hovered||selected) t.color(0xffffffff);
			//else						t.color(0xffe0e0e0);
			else t.color(0xffffffff);

			float hx = ((float) d.width) * 0.5f;
			float hy = ((float) d.height) * 0.5f;
			const float cx = ((float)x+d.x) + hx;
			const float cy = ((float)y+d.y) + hy;

			if (hovered)
				_buttonScale = Mth::Max(0.95f, _buttonScale-0.025f);
			else
				_buttonScale = Mth::Min(1.00f, _buttonScale+0.025f);

			hx *= _buttonScale;
			hy *= _buttonScale;

			const IntRectangle* src = _imageDef.getSrc();
			if (src) {
				const TextureData* d = minecraft->textures->getTemporaryTextureData(texId);
				if (d != NULL) {
					float u0 = (src->x+(hovered?src->w:0)) / (float)d->w;
					float u1 = (src->x+(hovered?2*src->w:src->w)) / (float)d->w;
					float v0 =  src->y / (float)d->h;
					float v1 = (src->y+src->h) / (float)d->h;
					t.vertexUV(cx-hx, cy-hy, blitOffset, u0, v0);
					t.vertexUV(cx-hx, cy+hy, blitOffset, u0, v1);
					t.vertexUV(cx+hx, cy+hy, blitOffset, u1, v1);
					t.vertexUV(cx+hx, cy-hy, blitOffset, u1, v0);
				}
			} else {
				t.vertexUV(cx-hx, cy-hy, blitOffset, 0, 0);
				t.vertexUV(cx-hx, cy+hy, blitOffset, 0, 1);
				t.vertexUV(cx+hx, cy+hy, blitOffset, 1, 1);
				t.vertexUV(cx+hx, cy-hy, blitOffset, 1, 0);
			}
		t.draw();
	}
	//blit(0, 0, 0, 0, 64, 64, 256, 256);

	//LOGI("%d %d\n", x+d.x, x+d.x+d.w);

	if (!active) {
		drawCenteredString(font, msg, x + width / 2, y + 11/*(h - 16)*/, 0xffa0a0a0);
	} else {
		if (hovered || selected) {
			drawCenteredString(font, msg, x + width / 2, y + 11/*(h - 16)*/, 0xffffa0);
		} else {
			drawCenteredString(font, msg, x + width / 2, y + 11/*(h - 48)*/, 0xe0e0e0);
		}
	}
}
