#include "DeathScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/time.h"

static const int WAIT_TICKS = 30;

DeathScreen::DeathScreen()
:	bRespawn(0),
	bTitle(0),
	_hasChosen(false),
	_tick(0)
{
}

DeathScreen::~DeathScreen()
{
	delete bRespawn;
	delete bTitle;
}

void DeathScreen::init()
{
	if (minecraft->useTouchscreen()) {
		bRespawn = new Touch::TButton(1, "Respawn!");
		bTitle = new Touch::TButton(2, "Main menu");
	} else {
		bRespawn = new Button(1, "Respawn!");
		bTitle = new Button(2, "Main menu");
	}
	buttons.push_back(bRespawn);
	buttons.push_back(bTitle);

	tabButtons.push_back(bRespawn);
	tabButtons.push_back(bTitle);
}

void DeathScreen::setupPositions()
{
	bRespawn->width = bTitle->width = width / 4;

	bRespawn->y = bTitle->y = height / 2;
	bRespawn->x = width/2 - bRespawn->width - 10;
	bTitle->x = width/2 + 10;

	LOGI("xyz: %d, %d (%d, %d)\n", bTitle->x, bTitle->y, width, height);
}

void DeathScreen::tick() {
	++_tick;
}

void DeathScreen::render( int xm, int ym, float a )
{
	fillGradient(0, 0, width, height, 0x60500000, 0xa0803030);

	glPushMatrix2();
	glScalef2(2, 2, 2);
	drawCenteredString(font, "You died!", width / 2 / 2, height / 8, 0xffffff);
	glPopMatrix2();

	if (_tick >= WAIT_TICKS)
		Screen::render(xm, ym, a);
}

void DeathScreen::buttonClicked( Button* button )
{
	if (_tick < WAIT_TICKS) return;

	if (button == bRespawn) {
		//RespawnPacket packet();
		//minecraft->raknetInstance->send(packet);

		minecraft->player->respawn();
		//minecraft->raknetInstance->send();
		minecraft->setScreen(NULL);
	}

	if (button == bTitle)
		minecraft->leaveGame();
}
