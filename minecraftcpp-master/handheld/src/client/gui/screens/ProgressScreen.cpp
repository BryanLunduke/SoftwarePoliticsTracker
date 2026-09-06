#include "ProgressScreen.h"
#include "DisconnectionScreen.h"
#include "../Gui.h"
#include "../Font.h"
#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../../SharedConstants.h"
#include "../../renderer/Textures.h"

ProgressScreen::ProgressScreen()
:	ticks(0)
{
}

void ProgressScreen::render( int xm, int ym, float a )
{
	if (minecraft->isLevelGenerated()) {
		minecraft->setScreen(NULL);
		return;
	}

	Tesselator& t = Tesselator::instance;
	renderBackground();

	minecraft->textures->loadAndBindTexture("gui/background.png");

	const float s = 32;
	t.begin();
	t.color(0x404040);
	t.vertexUV(0, (float)height, 0, 0, height / s);
	t.vertexUV((float)width, (float)height, 0, width / s, height / s);
	t.vertexUV((float)width, 0, 0, width / s, 0);
	t.vertexUV(0, 0, 0, 0, 0);
	t.draw();

	int i = minecraft->progressStagePercentage;

	if (i >= 0) {
		int w = 100;
		int h = 2;
		int x = width / 2 - w / 2;
		int y = height / 2 + 16;

		//printf("%d, %d - %d, %d\n", x, y, x + w, y + h);

		glDisable2(GL_TEXTURE_2D);
		t.begin();
		t.color(0x808080);
		t.vertex((float)x, (float)y, 0);
		t.vertex((float)x, (float)(y + h), 0);
		t.vertex((float)(x + w), (float)(y + h), 0);
		t.vertex((float)(x + w), (float)y, 0);

		t.color(0x80ff80);
		t.vertex((float)x, (float)y, 0);
		t.vertex((float)x, (float)(y + h), 0);
		t.vertex((float)(x + i), (float)(y + h), 0);
		t.vertex((float)(x + i), (float)y, 0);
		t.draw();
		glEnable2(GL_TEXTURE_2D);
	}

    glEnable2(GL_BLEND);

	const char* title = "Generating world";
	minecraft->font->drawShadow(title, (float)((width - minecraft->font->width(title)) / 2), (float)(height / 2 - 4 - 16), 0xffffff);

	const char* status = minecraft->getProgressMessage();
	const int progressWidth = minecraft->font->width(status);
	const int progressLeft  = (width - progressWidth) / 2;
	const int progressY = height / 2 - 4 + 8;
	minecraft->font->drawShadow(status, (float)progressLeft, (float)progressY, 0xffffff);

#if APPLE_DEMO_PROMOTION
	drawCenteredString(minecraft->font, "This demonstration version", width/2, progressY + 36, 0xffffff);
    drawCenteredString(minecraft->font, "does not allow saving games", width/2, progressY + 46, 0xffffff);
#endif
    
	// If we're locating the server, show our famous spinner!
	bool isLocating = (minecraft->getProgressStatusId() == 0);
	if (isLocating) {
		const int spinnerX = progressLeft + progressWidth + 6;
		static const char* spinnerTexts[] = {"-", "\\", "|", "/"};
		int n = ((int)(5.5f * getTimeS()) % 4);
		drawCenteredString(minecraft->font, spinnerTexts[n], spinnerX, progressY, 0xffffffff);
	}

    glDisable2(GL_BLEND);
	sleepMs(50);
}

bool ProgressScreen::isInGameScreen() { return false; }

void ProgressScreen::tick() {
	// After 10 seconds of not connecting -> write an error message and go back
	if (++ticks == 10 * SharedConstants::TicksPerSecond && minecraft->getProgressStatusId() == 0) {
		minecraft->setScreen( new DisconnectionScreen("Could not connect to server. Try again.") );
	}
}
