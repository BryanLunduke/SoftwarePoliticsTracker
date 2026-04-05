#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__BuyGameScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__BuyGameScreen_H__

#include "../Screen.h"
#include "../components/Button.h"

class BuyGameScreen: public Screen
{
public:
	BuyGameScreen() {}
	virtual ~BuyGameScreen() {}

	void init();

	void render(int xm, int ym, float a);

	void buttonClicked(Button* button) {
		//if (button->id == bQuit.id)
	}

private:
	//Button bQuit;
	//Button bBuyGame;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__BuyGameScreen_H__*/
