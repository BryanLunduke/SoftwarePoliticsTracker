#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__TextEditScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__TextEditScreen_H__

//package net.minecraft.client.gui;

#include "../Screen.h"
#include <string>
#include "../components/ImageButton.h"
class SignTileEntity;
class Button;
class TextEditScreen: public Screen
{
    typedef Screen super;
public:
	TextEditScreen(SignTileEntity* signEntity);
	~TextEditScreen();
    void init();
	void tick();
	bool handleBackEvent(bool isDown);
	void render(int xm, int ym, float a);
	virtual void lostFocus();
	virtual void keyPressed(int eventKey);
	virtual void keyboardNewChar(char inputChar);
	void setupPositions();
	void buttonClicked(Button* button);
protected:
	bool isShowingKeyboard;
	SignTileEntity* sign;
	int frame;
	int line;
private:
	ImageButton btnClose;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__TextEditScreen_H__*/
