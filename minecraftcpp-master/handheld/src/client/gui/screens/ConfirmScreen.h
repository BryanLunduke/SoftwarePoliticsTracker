#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__ConfirmScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__ConfirmScreen_H__

//package net.minecraft.client.gui;

#include "../Screen.h"
#include "../components/SmallButton.h"
#include <string>

class ConfirmScreen: public Screen
{
    typedef Screen super;
public:
    ConfirmScreen(Screen* parent_, const std::string& title1_, const std::string& title2_, int id_);
    ConfirmScreen(Screen* parent_, const std::string& title1_, const std::string& title2_, const std::string& yesButton, const std::string& noButton, int id_);
	~ConfirmScreen();

    void init();
	void setupPositions();

	bool handleBackEvent(bool isDown);
	void render(int xm, int ym, float a);
protected:
    void buttonClicked(Button* button);

	virtual void postResult(bool isOk);

    Screen* parent;
	int id;
private:
	std::string title1;
    std::string title2;

	std::string yesButtonText;
	std::string  noButtonText;

	Button* yesButton; // 0
	Button* noButton;  // 1
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__ConfirmScreen_H__*/
