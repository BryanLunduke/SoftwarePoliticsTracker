#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchJoinGameScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchJoinGameScreen_H__

#include "../../Screen.h"
#include "../../components/Button.h"
#include "../../components/SmallButton.h"
#include "../../../Minecraft.h"
#include "../../components/TextBox.h"

namespace Touch {

class JoinGameScreen: public Screen
{
public:
	JoinGameScreen();
	virtual ~JoinGameScreen();

	void init();
	void setupPositions();

	virtual bool handleBackEvent(bool isDown);

	virtual bool isIndexValid(int index);

	virtual void tick();
	void render(int xm, int ym, float a);

	void buttonClicked(Button* button);

	bool isInGameScreen();
private:
	TButton bJoin;
	TButton bBack;
	THeader bHeader;
	TextBox ip;
};

};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS_TOUCH__TouchJoinGameScreen_H__*/
