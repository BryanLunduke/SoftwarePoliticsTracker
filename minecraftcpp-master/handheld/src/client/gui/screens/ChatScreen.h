#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__

#include "../Screen.h"

class ChatScreen: public Screen
{
public:
	ChatScreen() {}
	virtual ~ChatScreen() {}

	void init();

	void render(int xm, int ym, float a);

	void buttonClicked(Button* button) {};

private:
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ChatScreen_H__*/
