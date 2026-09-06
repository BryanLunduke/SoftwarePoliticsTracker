#include "StartMenuScreen.h"
#include "SelectWorldScreen.h"
#include "ProgressScreen.h"
#include "JoinGameScreen.h"
#include "OptionsScreen.h"
#include "PauseScreen.h"
#include "InvalidLicenseScreen.h"
#include "PrerenderTilesScreen.h" // test button
//#include "BuyGameScreen.h"

#include "../../../util/Mth.h"

#include "../Font.h"
#include "../components/SmallButton.h"
#include "../components/ScrolledSelectionList.h"

#include "../../Minecraft.h"
#include "../../renderer/Tesselator.h"
#include "../../../AppPlatform.h"
#include "../../../LicenseCodes.h"
#include "SimpleChooseLevelScreen.h"
#include "../../renderer/Textures.h"
#include "../../../SharedConstants.h"

// Some kind of default settings, might be overridden in ::init
StartMenuScreen::StartMenuScreen()
:	bHost(    2, 0, 0, 160, 24, "Start Game"),
	bJoin(    3, 0, 0, 160, 24, "Join Game"),
	bOptions( 4, 0, 0,  78, 22, "Options"),
	bBuy(     5, 0, 0, 78, 22, "Buy"),
	bTest(    999, 0, 0, 78, 22, "Create")
{
}

StartMenuScreen::~StartMenuScreen()
{
}

void StartMenuScreen::init()
{
	buttons.push_back(&bHost);
	buttons.push_back(&bJoin);
	//buttons.push_back(&bTest);

	tabButtons.push_back(&bHost);
	tabButtons.push_back(&bJoin);

	#ifndef RPI
		buttons.push_back(&bOptions);
		tabButtons.push_back(&bOptions);
	#endif

	#ifdef DEMO_MODE
		buttons.push_back(&bBuy);
		tabButtons.push_back(&bBuy);
	#endif

	copyright = "\xffMojang AB";//. Do not distribute!";

	#ifdef PRE_ANDROID23
		std::string versionString = Common::getGameVersionString("j");
	#else
		std::string versionString = Common::getGameVersionString();
	#endif

	#ifdef DEMO_MODE
	#ifdef __APPLE__
		version = versionString + " (Lite)";
	#else
		version = versionString + " (Demo)";
	#endif
	#else
		#ifdef RPI
			version = "v0.1.1 alpha";//(MCPE " + versionString + " compatible)";
		#else
			version = versionString;
		#endif
	#endif

	bJoin.active = bHost.active = bOptions.active = false;
}

void StartMenuScreen::setupPositions() {
	int yBase = height / 2 + 25;

	//#ifdef ANDROID
	bHost.y =	 yBase - 28;
#ifdef RPI
	bJoin.y =	 yBase + 4;
#else
	bJoin.y =	 yBase;
#endif

	bOptions.y = yBase + 28 + 2;
	bTest.y = bBuy.y = bOptions.y;
	//#endif

	// Center buttons
	bHost.x = (width - bHost.width) / 2;
	bJoin.x = (width - bJoin.width) / 2;
	bOptions.x = (width - bJoin.width) / 2;
	bTest.x = bBuy.x = bOptions.x + bOptions.width + 4;

	copyrightPosX = width - minecraft->font->width(copyright) - 1;
	versionPosX = (width - minecraft->font->width(version)) / 2;// - minecraft->font->width(version) - 2;
}

void StartMenuScreen::tick() {
	_updateLicense();
}

void StartMenuScreen::buttonClicked(Button* button) {

	if (button->id == bHost.id)
	{
        #if defined(DEMO_MODE) || defined(APPLE_DEMO_PROMOTION)
			minecraft->setScreen( new SimpleChooseLevelScreen("_DemoLevel") );
		#else
			minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
		#endif
	}
	if (button->id == bJoin.id)
	{
		minecraft->locateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
	}
	if (button->id == bOptions.id)
	{
		minecraft->setScreen(new OptionsScreen());
	}
	if (button->id == bTest.id)
	{
		//minecraft->setScreen(new PauseScreen());
		//minecraft->setScreen(new PrerenderTilesScreen());
	}
	if (button->id == bBuy.id)
	{
		minecraft->platform()->buyGame();
		//minecraft->setScreen(new BuyGameScreen());
	}
}

bool StartMenuScreen::isInGameScreen() { return false; }

void StartMenuScreen::render( int xm, int ym, float a )
{
	renderBackground();

#if defined(RPI)
	TextureId id = minecraft->textures->loadTexture("gui/pi_title.png");
#else
	TextureId id = minecraft->textures->loadTexture("gui/title.png");
#endif
	const TextureData* data = minecraft->textures->getTemporaryTextureData(id);

	if (data) {
		minecraft->textures->bind(id);

		const float x = (float)width / 2;
		const float y = 4;
		//const float scale = Mth::Min(
		const float wh = Mth::Min((float)width/2.0f, (float)data->w / 2);
		const float scale = 2.0f * wh / (float)data->w;
		const float h = scale * (float)data->h;

		// Render title text
		Tesselator& t = Tesselator::instance;
		glColor4f2(1, 1, 1, 1);
		t.begin();
		t.vertexUV(x-wh, y+h, blitOffset, 0, 1);
		t.vertexUV(x+wh, y+h, blitOffset, 1, 1);
		t.vertexUV(x+wh, y+0, blitOffset, 1, 0);
		t.vertexUV(x-wh, y+0, blitOffset, 0, 0);
		t.draw();
	}

#if defined(RPI)
	if (Textures::isTextureIdValid(minecraft->textures->loadAndBindTexture("gui/logo/raknet_high_72.png")))
		blit(0, height - 12, 0, 0, 43, 12, 256, 72+72);
#endif

	drawString(font, version, versionPosX, 62, /*50,*/ 0xffcccccc);//0x666666);
	drawString(font, copyright, copyrightPosX, height - 10, 0xffffff);

	Screen::render(xm, ym, a);
}

void StartMenuScreen::_updateLicense()
{
	int id = minecraft->getLicenseId();
	if (LicenseCodes::isReady(id))
	{
		if (LicenseCodes::isOk(id))
			bJoin.active = bHost.active = bOptions.active = true;
		else
		{
			bool hasBuyButton = minecraft->platform()->hasBuyButtonWhenInvalidLicense();
			minecraft->setScreen(new InvalidLicenseScreen(id, hasBuyButton));
		}
	} else {
		bJoin.active = bHost.active = bOptions.active = false;
	}
}

bool StartMenuScreen::handleBackEvent( bool isDown ) {
	minecraft->quit();
	return true;
}
