#include "TouchStartMenuScreen.h"
#include "../ProgressScreen.h"
#include "../OptionsScreen.h"
#include "../PauseScreen.h"
#include "../InvalidLicenseScreen.h"
//#include "BuyGameScreen.h"

#include "../../Font.h"
#include "../../components/SmallButton.h"
#include "../../components/ScrolledSelectionList.h"
#include "../../components/GuiElement.h"

#include "../../../Minecraft.h"
#include "../../../renderer/Tesselator.h"
#include "../../../renderer/Textures.h"
#include "../../../renderer/TextureData.h"
#include "../../../../SharedConstants.h"
#include "../../../../AppPlatform.h"
#include "../../../../LicenseCodes.h"
#include "../../../../util/Mth.h"

#include "../DialogDefinitions.h"
#include "../SimpleChooseLevelScreen.h"

//
// Buy Button implementation
//
BuyButton::BuyButton(int id)
:	super(id, "")
{
	ImageDef def;
	// Setup the source rectangle
	def.setSrc(IntRectangle(64, 182, 190, 55));
	def.width = 75;//rc.w / 3;
	def.height = 75 * (55.0f / 190.0f);//rc.h / 3;
	def.name = "gui/gui.png";

	setImageDef(def, true);
}

void BuyButton::render(Minecraft* minecraft, int xm, int ym) {
	glColor4f2(1, 1, 1, 1);
	bool hovered = active && (minecraft->useTouchscreen()? (xm >= x && ym >= y && xm < x + width && ym < y + height) : false);
	renderBg(minecraft, xm, ym);
	TextureId texId = (_imageDef.name.length() > 0)? minecraft->textures->loadAndBindTexture(_imageDef.name) : Textures::InvalidId;
	if ( Textures::isTextureIdValid(texId) ) {
		const ImageDef& d = _imageDef;
		Tesselator& t = Tesselator::instance;
		
		t.begin();
			if (!active)				t.color(0xff808080);
			else if (hovered||selected) t.color(0xffcccccc);
			//else						t.color(0xffe0e0e0);
			else t.color(0xffffffff);

			float hx = ((float) d.width) * 0.5f;
			float hy = ((float) d.height) * 0.5f;
			const float cx = ((float)x+d.x) + hx;
			const float cy = ((float)y+d.y) + hy;
			if (hovered) {
				hx *= 0.95f;
				hy *= 0.95f;
			}

			const TextureData* td = minecraft->textures->getTemporaryTextureData(texId);
			const IntRectangle* src = _imageDef.getSrc();
			if (td != NULL && src != NULL) {
				float u0 = (src->x) / (float)td->w;
				float u1 = (src->x+src->w) / (float)td->w;
				float v0 = (src->y) / (float)td->h;
				float v1 = (src->y+src->h) / (float)td->h;
				t.vertexUV(cx-hx, cy-hy, blitOffset, u0, v0);
				t.vertexUV(cx-hx, cy+hy, blitOffset, u0, v1);
				t.vertexUV(cx+hx, cy+hy, blitOffset, u1, v1);
				t.vertexUV(cx+hx, cy-hy, blitOffset, u1, v0);
			}
		t.draw();
	}
}

namespace Touch {

// 
// Start menu screen implementation
//

// Some kind of default settings, might be overridden in ::init
StartMenuScreen::StartMenuScreen()
:	bHost(    2, "Start Game"),
	bJoin(    3, "Join Game"),
	bOptions( 4, "Options"),
	bBuy(     5),
	bTest(    9, "Create")
{
	ImageDef def;
	bJoin.width = 75;
	def.width = def.height = (float) bJoin.width;

	def.setSrc(IntRectangle(0, 26, (int)def.width, (int)def.width));
	def.name = "gui/touchgui.png";
	IntRectangle& defSrc = *def.getSrc();

	bOptions.setImageDef(def, true);

	defSrc.y += defSrc.h;
	bHost.setImageDef(def, true);

	defSrc.y += defSrc.h;
	bJoin.setImageDef(def, true);
}

StartMenuScreen::~StartMenuScreen()
{
}

void StartMenuScreen::init()
{
	buttons.push_back(&bHost);
	buttons.push_back(&bJoin);
	buttons.push_back(&bOptions);
    
    //buttons.push_back(&bTest);

	tabButtons.push_back(&bHost);
	tabButtons.push_back(&bJoin);
	tabButtons.push_back(&bOptions);

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
		version = versionString;
	#endif
    
    #ifdef APPLE_DEMO_PROMOTION
        version = versionString + " (Demo)";
    #endif

	bJoin.active = bHost.active = bOptions.active = false;
}

void StartMenuScreen::setupPositions() {
	int yBase = 2 + height / 3;
	int buttonWidth = bHost.width;
	float spacing = (width - (3.0f * buttonWidth)) / 4;

	//#ifdef ANDROID
	bHost.y =	 yBase;
	bJoin.y =	 yBase;
	bOptions.y = yBase;
	//#endif

	//bTest.x = 0; //width - bTest.w;
	//bTest.y = height - bTest.h;

	// Center buttons
	bJoin.x		= 0*buttonWidth + (int)(1*spacing);
	bHost.x		= 1*buttonWidth + (int)(2*spacing);
	bOptions.x	= 2*buttonWidth + (int)(3*spacing);
	//bBuy.y = bOptions.y - bBuy.h - 6;
	//bBuy.x = bOptions.x + bOptions.w - bBuy.w;

	bBuy.y = height - bBuy.height - 3;
	bBuy.x = (width - bBuy.width) / 2;
    
    bTest.x = 4;
    bTest.y = height - bTest.height - 4;

	copyrightPosX = width - minecraft->font->width(copyright) - 1;
	versionPosX = (width - minecraft->font->width(version)) / 2;// - minecraft->font->width(version) - 2;
}

void StartMenuScreen::tick() {
	_updateLicense();
}

void StartMenuScreen::buttonClicked(::Button* button) {

	//if (button->id == bTest.id) {
	//	minecraft->selectLevel("Broken", "Broken", 1317199248);
	//	minecraft->hostMultiplayer();
	//	minecraft->setScreen(new ProgressScreen());
	//}
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
        #ifdef APPLE_DEMO_PROMOTION
            minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_DEMO_FEATURE_DISABLED);
        #else
            minecraft->locateMultiplayer();
            minecraft->screenChooser.setScreen(SCREEN_JOINGAME);
        #endif
	}
	if (button->id == bOptions.id)
	{
		minecraft->setScreen(new OptionsScreen());
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
    
    glEnable2(GL_BLEND);

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
		const float wh = 0.5f * Mth::Min((float)width/2.0f, (float)data->w / 2);
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

		drawString(font, version, versionPosX, (int)(y+h)+2, /*50,*/ 0xffcccccc);//0x666666);
		drawString(font, copyright, copyrightPosX, height - 10, 0xffffff);
		//patch->draw(t, 0, 20);
	}
	Screen::render(xm, ym, a);
    glDisable2(GL_BLEND);
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

};
