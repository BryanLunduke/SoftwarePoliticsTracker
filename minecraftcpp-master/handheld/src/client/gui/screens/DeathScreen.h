#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__DeathScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__DeathScreen_H__

#include "../Screen.h"
class Button;

class DeathScreen: public Screen
{
public:
	DeathScreen();

	virtual ~DeathScreen();

	void init();

	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);

	void buttonClicked(Button* button);

private:
	Button* bRespawn;
	Button* bTitle;
	bool _hasChosen;
	int _tick;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__DeathScreen_H__*/
