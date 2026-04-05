#include "OptionsScreen.h"

#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"

#include "../components/OptionsPane.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
OptionsScreen::OptionsScreen()
: btnClose(NULL),
  bHeader(NULL),
  selectedCategory(0) {
}

OptionsScreen::~OptionsScreen() {
	if(btnClose != NULL) {
		delete btnClose;
		btnClose = NULL;
	}
	if(bHeader != NULL) {
		delete bHeader,
		bHeader = NULL;
	}
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if(*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	categoryButtons.clear();
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");
	btnClose = new ImageButton(1, "");
	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	categoryButtons.push_back(new Touch::TButton(2, "Login"));
	categoryButtons.push_back(new Touch::TButton(3, "Game"));
	categoryButtons.push_back(new Touch::TButton(4, "Controls"));
	categoryButtons.push_back(new Touch::TButton(5, "Graphics"));
	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}
	generateOptionScreens();

}
void OptionsScreen::setupPositions() {
	int buttonHeight = btnClose->height;
	btnClose->x = width - btnClose->width;
	btnClose->y = 0;
	int offsetNum = 1;
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		(*it)->x = 0;
		(*it)->y = offsetNum * buttonHeight;
		(*it)->selected = false;
		offsetNum++;
	}
	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = btnClose->height;
	for(std::vector<OptionsPane*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if(categoryButtons.size() > 0 && categoryButtons[0] != NULL) {
			(*it)->x = categoryButtons[0]->width;
			(*it)->y = bHeader->height;
			(*it)->width = width - categoryButtons[0]->width;
			(*it)->setupPositions();
		}
	}
	selectCategory(0);
}

void OptionsScreen::render( int xm, int ym, float a ) {
	renderBackground();
	super::render(xm, ym, a);
	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;
	if(currentOptionPane != NULL)
		currentOptionPane->render(minecraft, xmm, ymm);
}

void OptionsScreen::removed()
{
}
void OptionsScreen::buttonClicked( Button* button ) {
	if(button == btnClose) {
		minecraft->reloadOptions();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	} else if(button->id > 1 && button->id < 7) {
		// This is a category button
		int categoryButton = button->id - categoryButtons[0]->id;
		selectCategory(categoryButton);
	}
}

void OptionsScreen::selectCategory( int index ) {
	int currentIndex = 0;
	for(std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if(index == currentIndex) {
			(*it)->selected = true;
		} else {
			(*it)->selected = false;
		}
		currentIndex++;
	}
	if(index < (int)optionPanes.size())
		currentOptionPane = optionPanes[index];
}

void OptionsScreen::generateOptionScreens() {
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	optionPanes.push_back(new OptionsPane());
	// Mojang Pane
	optionPanes[0]->createOptionsGroup("options.group.mojang")
		//.addOptionItem(&Options::Option::THIRD_PERSON, minecraft);
		.addOptionItem(&Options::Option::SENSITIVITY, minecraft);
// 	int mojangGroup = optionPanes[0]->createOptionsGroup("Mojang");
// 	static const int arr[] = {5,4,3,15};
// 	std::vector<int> vec (arr, arr + sizeof(arr) / sizeof(arr[0]) );
// 	optionPanes[0]->createStepSlider(minecraft, mojangGroup, "This works?", &Options::Option::DIFFICULTY, vec);
// 
// 	// Game Pane
// 	int gameGroup = optionPanes[1]->createOptionsGroup("Game");
// 	optionPanes[1]->createToggle(gameGroup, "Third person camera", &Options::Option::THIRD_PERSON);
// 	optionPanes[1]->createToggle(gameGroup, "Server visible", &Options::Option::SERVER_VISIBLE);
// 	
// 	// Input Pane
// 	int controlsGroup = optionPanes[2]->createOptionsGroup("Controls");
// 	optionPanes[2]->createToggle(controlsGroup, "Invert X-axis", &Options::Option::INVERT_MOUSE);
// 	optionPanes[2]->createToggle(controlsGroup, "Lefty", &Options::Option::LEFT_HANDED);
// 	optionPanes[2]->createToggle(controlsGroup, "Use touch screen", &Options::Option::USE_TOUCHSCREEN);
// 	optionPanes[2]->createToggle(controlsGroup, "Split touch controls", &Options::Option::USE_TOUCH_JOYPAD);
// 	int feedBackGroup = optionPanes[2]->createOptionsGroup("Feedback");
// 	optionPanes[2]->createToggle(feedBackGroup, "Vibrate on destroy", &Options::Option::DESTROY_VIBRATION);
// 
// 	int graphicsGroup = optionPanes[3]->createOptionsGroup("Graphics");
// 	optionPanes[3]->createProgressSlider(minecraft, graphicsGroup, "Gui Scale", &Options::Option::PIXELS_PER_MILLIMETER, 3, 4);
// 	optionPanes[3]->createToggle(graphicsGroup, "Fancy Graphics", &Options::Option::INVERT_MOUSE);
// 	optionPanes[3]->createToggle(graphicsGroup, "Fancy Skies", &Options::Option::INVERT_MOUSE);
// 	optionPanes[3]->createToggle(graphicsGroup, "Animated water", &Options::Option::INVERT_MOUSE);
// 	int experimentalGraphicsGroup = optionPanes[3]->createOptionsGroup("Experimental graphics");
// 	optionPanes[3]->createToggle(experimentalGraphicsGroup, "Soft shadows", &Options::Option::INVERT_MOUSE);
}

void OptionsScreen::mouseClicked( int x, int y, int buttonNum ) {
	if(currentOptionPane != NULL)
		currentOptionPane->mouseClicked(minecraft, x, y, buttonNum);
	super::mouseClicked(x, y, buttonNum);
}

void OptionsScreen::mouseReleased( int x, int y, int buttonNum ) {
	if(currentOptionPane != NULL)
		currentOptionPane->mouseReleased(minecraft, x, y, buttonNum);
	super::mouseReleased(x, y, buttonNum);
}

void OptionsScreen::tick() {
	if(currentOptionPane != NULL)
		currentOptionPane->tick(minecraft);
	super::tick();
}
