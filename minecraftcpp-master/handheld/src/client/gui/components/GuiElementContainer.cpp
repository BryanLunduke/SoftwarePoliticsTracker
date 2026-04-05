#include "GuiElementContainer.h"
#include <algorithm>
GuiElementContainer::GuiElementContainer( bool active/*=false*/, bool visible/*=true*/, int x /*= 0*/, int y /*= 0*/, int width/*=24*/, int height/*=24*/ )
: GuiElement(active, visible, x, y, width, height) {

}

GuiElementContainer::~GuiElementContainer() {
	while(!children.empty()) {
		GuiElement* element = children.back();
		children.pop_back();
		delete element;
	}
}

void GuiElementContainer::render( Minecraft* minecraft, int xm, int ym ) {
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->render(minecraft, xm, ym);
	}
}

void GuiElementContainer::setupPositions() {
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->setupPositions();
	}
}

void GuiElementContainer::addChild( GuiElement* element ) {
	children.push_back(element);
}

void GuiElementContainer::removeChild( GuiElement* element ) {
	std::vector<GuiElement*>::iterator it = std::find(children.begin(), children.end(), element);
	if(it != children.end())
		children.erase(it);
}

void GuiElementContainer::tick( Minecraft* minecraft ) {
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->tick(minecraft);
	}
}

void GuiElementContainer::mouseClicked( Minecraft* minecraft, int x, int y, int buttonNum ) {
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->mouseClicked(minecraft, x, y, buttonNum);
	}
}

void GuiElementContainer::mouseReleased( Minecraft* minecraft, int x, int y, int buttonNum ) {
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->mouseReleased(minecraft, x, y, buttonNum);
	}
}
