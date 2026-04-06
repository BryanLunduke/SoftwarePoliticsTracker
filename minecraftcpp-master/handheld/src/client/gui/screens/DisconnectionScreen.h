#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DisconnectionScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DisconnectionScreen_H__

#include "../Screen.h"
#include "../Font.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include <string>

class DisconnectionScreen: public Screen
{
	typedef Screen super;
public:
	DisconnectionScreen(const std::string& msg)
	:	_msg(msg),
		_back(NULL)
	{}

	~DisconnectionScreen() {
		delete _back;
	}

	void init() {
		if (minecraft->useTouchscreen())
			_back = new Touch::TButton(1, "Ok");
		else
			_back = new Button(1, "Ok");

		buttons.push_back(_back);
		tabButtons.push_back(_back);

		_back->width = 128;
		_back->x = (width - _back->width) / 2;
		_back->y = height / 2;
	}

	void render( int xm, int ym, float a ) {
		renderBackground();
		super::render(xm, ym, a);

		int center = (width - minecraft->font->width(_msg)) / 2;
		minecraft->font->drawShadow(_msg, (float)center, (float)(height / 2 - 32), 0xffffffff);
	}

	void buttonClicked(Button* button) {
		if (button->id == _back->id) {
			minecraft->leaveGame();
		}
	}
	bool isInGameScreen() { return false; }

private:
	std::string _msg;
	Button* _back;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__DisconnectionScreen_H__*/
