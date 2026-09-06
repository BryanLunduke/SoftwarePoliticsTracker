#include "GuiElement.h"

GuiElement::GuiElement( bool active/*=false*/, bool visible/*=true*/, int x /*= 0*/, int y /*= 0*/, int width/*=24*/, int height/*=24*/ )
: active(active),
  visible(visible),
  x(x),
  y(y),
  width(width),
  height(height) {
		
}

bool GuiElement::pointInside( int x, int y ) {
	if(x >= this->x && x < this->x + this->width) {
		if(y >= this->y && y < this->y + this->height) {
			return true;
		}
	}
	return false;
}
