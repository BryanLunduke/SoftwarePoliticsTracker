#include "ImageButton.h"
#include "../../renderer/Tesselator.h"
#include "../../Minecraft.h"
#include "../../../platform/log.h"
#include "../../../util/Mth.h"
#include "../../renderer/Textures.h"


ImageButton::ImageButton(int id, const std::string& msg)
:	super(id, msg)
{
	setupDefault();
}

ImageButton::ImageButton(int id, const std::string& msg, const ImageDef& imagedef)
:	super(id, msg),
	_imageDef(imagedef)
{
	setupDefault();
}

void ImageButton::setupDefault() {
	width = 48;
	height = 48;
	scaleWhenPressed = true;
}

void ImageButton::setImageDef(const ImageDef& imageDef, bool setButtonSize) {
	_imageDef = imageDef;
	if (setButtonSize) {
		width = (int)_imageDef.width;
		height = (int)_imageDef.height;
	}
}

void ImageButton::render(Minecraft* minecraft, int xm, int ym) {
	if (!visible) return;

	Font* font = minecraft->font;

	//minecraft->textures->loadAndBindTexture("gui/gui.png");
	glColor4f2(1, 1, 1, 1);

	bool hovered = active && (minecraft->useTouchscreen()? (_currentlyDown && xm >= x && ym >= y && xm < x + width && ym < y + height) : false);
	bool IsSecondImage = isSecondImage(hovered);

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
			if (scaleWhenPressed && hovered) {
				hx *= 0.95f;
				hy *= 0.95f;
			}

			const IntRectangle* src = _imageDef.getSrc();
			if (src) {
				const TextureData* d = minecraft->textures->getTemporaryTextureData(texId);
				if (d != NULL) {
					float u0 = (src->x+(IsSecondImage?src->w:0)) / (float)d->w;
					float u1 = (src->x+(IsSecondImage?2*src->w:src->w)) / (float)d->w;
					float v0 = src->y / (float)d->h;
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
		drawCenteredString(font, msg, x + width / 2, y + 16/*(h - 16)*/, 0xffa0a0a0);
	} else {
		if (hovered || selected) {
			drawCenteredString(font, msg, x + width / 2, y + 17/*(h - 16)*/, 0xffffa0);
		} else {
			drawCenteredString(font, msg, x + width / 2, y + 16/*(h - 48)*/, 0xe0e0e0);
		}
	}
}


//
// A toggleable Button
//
OptionButton::OptionButton(const Options::Option* option)
:	_option(option),
	_isFloat(false),
	super(ButtonId, "")
{
}

OptionButton::OptionButton(const Options::Option* option, float onValue, float offValue)
:	_option(option),
	_isFloat(true),
	_onValue(onValue),
	_offValue(offValue),
	super(ButtonId, "")
{
}

bool OptionButton::isSecondImage(bool hovered) {
	return _secondImage;
}

void OptionButton::toggle(Options* options) {
	if (_isFloat) {
		options->set(_option, (Mth::abs(_current - _onValue) < 0.01f) ? _offValue : _onValue);
	} else {
		options->toggle(_option, 1);
	}
	// Update graphics here
	updateImage(options);
}

void OptionButton::updateImage(Options* options) {
	if (_isFloat) {
		_current = options->getProgressValue(_option);
		_secondImage = Mth::abs(_current - _onValue) < 0.01f;
	} else {
		_secondImage = options->getBooleanValue(_option);
	}
}

void OptionButton::mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum ) {
	if(buttonNum == MouseAction::ACTION_LEFT) {
		if(clicked(minecraft, x, y)) {
			toggle(&minecraft->options);
		}
	}
}
