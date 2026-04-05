#ifndef NET_MINECRAFT_CLIENT_GUI__GButton_H__
#define NET_MINECRAFT_CLIENT_GUI__GButton_H__
#include "Button.h"

class GButton: public Button {
	typedef Button super;
public:
	static const int LayerDefault	= 1;
	static const int LayerSelected  = 2;
	static const int LayerMax       = 4;

	GButton(int id)
		:	super(id, "")
	{}
	~GButton() {
		for (unsigned int i = 0; i < layers.size(); ++i) {
			delete layers[i].first;
		}
	}

	void addElement(int layerId, GuiElement* e) {
		if (!e || layerId < 0 || layerId >= LayerMax) {
			LOGE("Error @ GButton::element : Trying to add element %p at layer: %d\n", e, layerId);
			return;
		}
		layers.push_back(std::make_pair(e, layerId));
	}

	void render( Minecraft* minecraft, int xm, int ym )
	{
		if (!visible) return;

		bool isHovered = minecraft->isTouchscreen()?
			(_currentlyDown && xm >= x && ym >= y && xm < x + width && ym < y + height): false;

		int layer = isHovered? LayerSelected : LayerDefault;
		if (layer < 0) return;

		Tesselator& t = Tesselator::instance;
		t.addOffset((float)x, (float)y, 0);

		for (unsigned int i = 0; i < layers.size(); ++i) {
			if ((layers[i].second & layer) != 0)
				layers[i].first->render(minecraft, 0, 0);
		}

		t.addOffset((float)-x, (float)-y, 0);
	}

private:
	std::vector<std::pair<GuiElement*, int> > layers;
};


#endif /*NET_MINECRAFT_CLIENT_GUI__GButton_H__*/
