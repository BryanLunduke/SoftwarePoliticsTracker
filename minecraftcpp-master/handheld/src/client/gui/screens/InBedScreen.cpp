#include "InBedScreen.h"
#include "ScreenChooser.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../platform/time.h"

static const int WAIT_TICKS = 30;

InBedScreen::InBedScreen()
	: bWakeUp(0)
{
}

InBedScreen::~InBedScreen() {
	delete bWakeUp;
}

void InBedScreen::init() {
	if (minecraft->useTouchscreen()) {
		bWakeUp = new Touch::TButton(1, "Leave Bed");
	} else {
		bWakeUp = new Button(1, "Leave Bed");
	}
	buttons.push_back(bWakeUp);

	tabButtons.push_back(bWakeUp);
}

void InBedScreen::setupPositions() {
	bWakeUp->width = width / 2;
	bWakeUp->height = int(height * 0.2f);
	bWakeUp->y = height - int(bWakeUp->height * 1.5);
	bWakeUp->x = width/2 - bWakeUp->width/2;
}

void InBedScreen::render( int xm, int ym, float a ) {
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	Screen::render(xm, ym, a);
	glDisable(GL_BLEND);
}

void InBedScreen::buttonClicked( Button* button ) {
	if (button == bWakeUp) {
		minecraft->player->stopSleepInBed(true, true, true);
		minecraft->setScreen(NULL);
	}
}
