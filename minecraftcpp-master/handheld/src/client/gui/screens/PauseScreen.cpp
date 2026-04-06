#include "PauseScreen.h"
#include "StartMenuScreen.h"
#include "../components/ImageButton.h"
#include "../../Minecraft.h"
#include "../../../util/Mth.h"
#include "../../../network/RakNetInstance.h"
#include "../../../network/ServerSideNetworkHandler.h"

PauseScreen::PauseScreen(bool wasBackPaused)
:	saveStep(0),
	visibleTime(0),
	bContinue(0),
	bQuit(0),
	bQuitAndSaveLocally(0),
	bServerVisibility(0),
//	bThirdPerson(0),
	wasBackPaused(wasBackPaused),
	bSound(&Options::Option::SOUND, 1, 0),
	bThirdPerson(&Options::Option::THIRD_PERSON),
    bHideGui(&Options::Option::HIDE_GUI)
{
	ImageDef def;
	def.setSrc(IntRectangle(160, 144, 39, 31));
	def.name = "gui/touchgui.png";
	IntRectangle& defSrc = *def.getSrc();

	def.width = defSrc.w * 0.666667f;
	def.height = defSrc.h * 0.666667f;

	bSound.setImageDef(def, true);
	defSrc.y += defSrc.h;
	bThirdPerson.setImageDef(def, true);
    bHideGui.setImageDef(def, true);
	//void setImageDef(ImageDef& imageDef, bool setButtonSize);
}

PauseScreen::~PauseScreen() {
	delete bContinue;
	delete bQuit;
	delete bQuitAndSaveLocally;
	delete bServerVisibility;
//	delete bThirdPerson;
}

void PauseScreen::init() {
	if (minecraft->useTouchscreen()) {
		bContinue = new Touch::TButton(1, "Back to game");
		bQuit = new Touch::TButton(2, "Quit to title");
		bQuitAndSaveLocally = new Touch::TButton(3, "Quit and copy map");
		bServerVisibility = new Touch::TButton(4, "");
//		bThirdPerson = new Touch::TButton(5, "Toggle 3:rd person view");
	} else {
		bContinue = new Button(1, "Back to game");
		bQuit = new Button(2, "Quit to title");
		bQuitAndSaveLocally = new Button(3, "Quit and copy map");
		bServerVisibility = new Button(4, "");
//		bThirdPerson = new Button(5, "Toggle 3:rd person view");
	}

	buttons.push_back(bContinue);
	buttons.push_back(bQuit);

	bSound.updateImage(&minecraft->options);
	bThirdPerson.updateImage(&minecraft->options);
	bHideGui.updateImage(&minecraft->options);
	buttons.push_back(&bSound);
	buttons.push_back(&bThirdPerson);
    //buttons.push_back(&bHideGui);

	// If Back wasn't pressed, set up additional items (more than Quit to menu
	// and Back to game) here
    
    #if !defined(APPLE_DEMO_PROMOTION) && !defined(RPI)
	if (true || !wasBackPaused) {
		if (minecraft->raknetInstance) {
			if (minecraft->raknetInstance->isServer()) {
				updateServerVisibilityText();
				buttons.push_back(bServerVisibility);
			}
			else {
                #if !defined(DEMO_MODE)
                buttons.push_back(bQuitAndSaveLocally);
				#endif
			}
		}
	}
    #endif
//	buttons.push_back(bThirdPerson);

	for (unsigned int i = 0; i < buttons.size(); ++i) {
		if (buttons[i] == &bSound) continue;
		if (buttons[i] == &bThirdPerson) continue;
		if (buttons[i] == &bHideGui) continue;
		tabButtons.push_back(buttons[i]);
	}
}

void PauseScreen::setupPositions() {
    saveStep = 0;
	int yBase = 16;

	bContinue->width = bQuit->width = /*bThirdPerson->w =*/ 160;
	bQuitAndSaveLocally->width = bServerVisibility->width = 160;

	bContinue->x = (width - bContinue->width) / 2;
	bContinue->y = yBase + 32 * 1;

	bQuit->x = (width - bQuit->width) / 2;
	bQuit->y = yBase + 32 * 2;

#if APPLE_DEMO_PROMOTION
    bQuit->y += 16;
#endif
    
	bQuitAndSaveLocally->x = bServerVisibility->x = (width - bQuitAndSaveLocally->width) / 2;
	bQuitAndSaveLocally->y = bServerVisibility->y = yBase + 32 * 3;

	bSound.y = bThirdPerson.y = 8;
	bSound.x = 4;
	bThirdPerson.x = bSound.x + 4 + bSound.width;
	bHideGui.x = bThirdPerson.x + 4 + bThirdPerson.width;

	//bThirdPerson->x = (width - bThirdPerson->w) / 2;
	//bThirdPerson->y = yBase + 32 * 4;
}

void PauseScreen::tick() {
	super::tick();
	visibleTime++;
}

void PauseScreen::render(int xm, int ym, float a) {
	renderBackground();

	//bool isSaving = !minecraft->level.pauseSave(saveStep++);
	//if (isSaving || visibleTime < 20) {
	//	float col = ((visibleTime % 10) + a) / 10.0f;
	//	col = Mth::sin(col * Mth::PI * 2) * 0.2f + 0.8f;
	//	int br = (int) (255 * col);

	//	drawString(font, "Saving level..", 8, height - 16, br << 16 | br << 8 | br);
	//}

	drawCenteredString(font, "Game menu", width / 2, 24, 0xffffff);

	super::render(xm, ym, a);
}

void PauseScreen::buttonClicked(Button* button) {
	if (button->id == bContinue->id) {
		minecraft->setScreen(NULL);
		//minecraft->grabMouse();
	}
    if (button->id == bQuit->id) {
		minecraft->leaveGame();
    }
	if (button->id == bQuitAndSaveLocally->id) {
		minecraft->leaveGame(true);
	}

	if (button->id == bServerVisibility->id) {
		if (minecraft->raknetInstance && minecraft->netCallback && minecraft->raknetInstance->isServer()) {
			ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) minecraft->netCallback;
			bool allows = !ss->allowsIncomingConnections();
			ss->allowIncomingConnections(allows);

			updateServerVisibilityText();
		}
	}

	if (button->id == OptionButton::ButtonId) {
		((OptionButton*)button)->toggle(&minecraft->options);
	}

	//if (button->id == bThirdPerson->id) {
	//	minecraft->options.thirdPersonView = !minecraft->options.thirdPersonView;
	//}
}

void PauseScreen::updateServerVisibilityText()
{
	if (!minecraft->raknetInstance || !minecraft->raknetInstance->isServer())
		return;

	ServerSideNetworkHandler* ss = (ServerSideNetworkHandler*) minecraft->netCallback;
	bServerVisibility->msg = ss->allowsIncomingConnections()?
		"Server is visible"
	:   "Server is invisible";
}
