#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__InvalidLicenseScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__InvalidLicenseScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../../LicenseCodes.h"

class InvalidLicenseScreen: public Screen
{
public:
	InvalidLicenseScreen(int id, bool hasBuyButton)
	:	_id(id),
		_hasBuyButton(hasBuyButton),
		_baseY(0),
		bOk(0),
		bBuy(0)
	{
	}

	virtual ~InvalidLicenseScreen() {
		delete bOk;
		delete bBuy;
	}

	void init() {
		if (minecraft->useTouchscreen()) {
			bOk  = new Touch::TButton(1, "Ok");
			bBuy = new Touch::TButton(2, "Buy");
		} else {
			bOk  = new Button(1, "Ok");
			bBuy = new Button(2, "Buy");
		}

		if (_hasBuyButton)
			bOk->msg = "Quit";

		if (!LicenseCodes::isOk(_id)) {
			char buf[20] = {0};
			sprintf(buf, "%d", _id);

			desc1 = "License verification failed (error ";
			desc1 += buf;
			desc1 += ")";
			desc2 = "Try again later.";
			hint =  "You need to be connected to the internet\n";
			hint += "once while you start the game.";
		}

		buttons.push_back(bOk);
		tabButtons.push_back(bOk);

		if (_hasBuyButton) {
			buttons.push_back(bBuy);
			tabButtons.push_back(bBuy);
		}
	}

	void setupPositions() {
		_baseY = height/5 + 6;
		//if (_hasBuyButton)
		_baseY -= 24;
		
		bOk->width = bBuy->width = 200;
		bOk->x = bBuy->x = (width - bOk->width) / 2;
		bBuy->y = _baseY + 84;
		bOk->y = bBuy->y + bBuy->height + 4;

		if (!_hasBuyButton)
			bOk->y -= 24;
	}

	void tick() {}

	//void keyPressed(int eventKey) {}

	void render(int xm, int ym, float a) {
		renderDirtBackground(0);
		drawCenteredString(minecraft->font, desc1, width/2, _baseY, 0xffffff);
		drawCenteredString(minecraft->font, desc2, width/2, _baseY + 24, 0xffffff);

		drawCenteredString(minecraft->font, hint, width/2, _baseY + 60, 0xffffff);

		Screen::render(xm, ym, a);
	}

	void buttonClicked(Button* button) {
		if (button->id == bOk->id) {
			minecraft->quit();
		}
		if (button->id == bBuy->id) {
			minecraft->platform()->buyGame();
		}
	};
private:
	int _id;
	std::string desc1;
	std::string desc2;
	std::string hint;

	Button* bOk;
	Button* bBuy;
	bool _hasBuyButton;
	int  _baseY;
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__InvalidLicenseScreen_H__*/
