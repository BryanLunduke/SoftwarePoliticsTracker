#ifndef NET_MINECRAFT_CLIENT_GUI__PauseScreen_H__
#define NET_MINECRAFT_CLIENT_GUI__PauseScreen_H__

//package net.minecraft.client.gui;

#include "../Screen.h"
#include "../components/ImageButton.h"

class Button;

class PauseScreen: public Screen
{
	typedef Screen super;
public:
	PauseScreen(bool wasBackPaused);
	~PauseScreen();

	void init();
	void setupPositions();

	void tick();
	void render(int xm, int ym, float a);
protected:
    void buttonClicked(Button* button);
private:
	void updateServerVisibilityText();

	int saveStep;
	int visibleTime;
	bool wasBackPaused;

	Button* bContinue;
	Button* bQuit;
	Button* bQuitAndSaveLocally;
	Button* bServerVisibility;
//	Button* bThirdPerson;

	OptionButton bSound;
	OptionButton bThirdPerson;
    OptionButton bHideGui;
};

#endif /*NET_MINECRAFT_CLIENT_GUI__PauseScreen_H__*/
