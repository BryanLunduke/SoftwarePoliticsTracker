#include "TouchSelectWorldScreen.h"
#include "../StartMenuScreen.h"
#include "../ProgressScreen.h"
#include "../DialogDefinitions.h"
#include "../../components/ImageButton.h" //weird!
#include "../../../renderer/Textures.h"
#include "../../../renderer/Tesselator.h"
#include "../../../../world/level/LevelSettings.h"
#include "../../../../AppPlatform.h"
#include "../../../../util/StringUtils.h"
#include "../../../../util/Mth.h"
#include "../../../../platform/input/Mouse.h"
#include "../../../../Performance.h"

#include <algorithm>
#include <set>
#include "../SimpleChooseLevelScreen.h"

namespace Touch {

//
// World Selection List
//
TouchWorldSelectionList::TouchWorldSelectionList( Minecraft* minecraft, int width, int height )
:	_height(height),
	hasPickedLevel(false),
	pickedIndex(-1),
	currentTick(0),
	stoppedTick(-1),
	mode(0),
	_newWorldSelected(false),
	RolledSelectionListH(minecraft, width, height, 0, width, 24, height-32, 120)
{
	_renderBottomBorder = false;
	//setRenderSelection(false);
}

int TouchWorldSelectionList::getNumberOfItems() {
	return (int)levels.size() + 1;
}

void TouchWorldSelectionList::selectItem( int item, bool doubleClick ) {
	if (selectedItem < 0)
		return;

	const int delta = item - selectedItem;
	
	if (delta == -1)
		stepLeft();
	if (delta == +1)
		stepRight();
	if (delta == 0 ) {
		if (!hasPickedLevel) {
			hasPickedLevel = true;
			pickedIndex = item;
			if (item < (int)levels.size())
				pickedLevel = levels[item];
		}
	}
}

bool TouchWorldSelectionList::isSelectedItem( int item ) {
	return item == selectedItem;
}

void TouchWorldSelectionList::selectStart(int item, int localX, int localY) {
	if (selectedItem != (int) levels.size() || item != selectedItem)
		return;
	_newWorldSelected = true;
}

void TouchWorldSelectionList::selectCancel() {
	_newWorldSelected = false;
}

void TouchWorldSelectionList::renderItem( int i, int x, int y, int h, Tesselator& t ) {
	int centerx = x + itemWidth/2;
	float a0 = Mth::Max(1.1f - std::abs( width / 2 - centerx ) * 0.0055f, 0.2f);
	if (a0 > 1) a0 = 1;
	int textColor =  (int)(255.0f * a0) * 0x010101;
	int textColor2 = (int)(140.0f * a0) * 0x010101;
	const int TX = centerx - itemWidth / 2 + 5;
	const int TY = y + 44; //@kindle-res:42

	if (i < (int)levels.size()) {
		// Draw the worlds
		StringVector v = _descriptions[i];
		drawString(minecraft->font, v[0].c_str(), TX, TY +  0, textColor);
		drawString(minecraft->font, v[1].c_str(), TX, TY + 10, textColor2);
		drawString(minecraft->font, v[2].c_str(), TX, TY + 20, textColor2);
		drawString(minecraft->font, v[3].c_str(), TX, TY + 30, textColor2);

		minecraft->textures->loadAndBindTexture(_imageNames[i]);
		t.color(0.3f, 1.0f, 0.2f);

		//float x0 = (float)x;
		//float x1 = (float)x + (float)itemWidth;

		const float IY = (float)y - 8; // @kindle-res: -3
		t.begin();
			t.color(textColor);
			t.vertexUV((float)(centerx-32), IY,      blitOffset, 0, 0.125f);
			t.vertexUV((float)(centerx-32), IY + 48, blitOffset, 0, 0.875f); //@kindle-res: +44
			t.vertexUV((float)(centerx+32), IY + 48, blitOffset, 1, 0.875f); //@kindle-res: +44
			t.vertexUV((float)(centerx+32), IY,      blitOffset, 1, 0.125f);
		t.draw();
	} else {
		// Draw the "Create new world" icon
		drawCenteredString(minecraft->font, "Create new", centerx, TY +  12, textColor);

		minecraft->textures->loadAndBindTexture("gui/touchgui.png");

		const bool selected = _newWorldSelected;

		const float W = 54.0f;
		const float H = 54.0f;
		const float IY = (float)y;
		const float u0 = (168.0f    ) / 256.0f;
		const float u1 = (168.0f + W) / 256.0f;
		float v0 = (32.0f     ) / 256.0f;
		float v1 = (32.0f  + H) / 256.0f;
		if (selected) {
			v0 += H / 256.0f;
			v1 += H / 256.0f;
		}

		t.begin();
		t.color(textColor);
		t.vertexUV((float)centerx - W*0.5f, IY,     blitOffset, u0, v0);
		t.vertexUV((float)centerx - W*0.5f, IY + H, blitOffset, u0, v1);
		t.vertexUV((float)centerx + W*0.5f, IY + H, blitOffset, u1, v1);
		t.vertexUV((float)centerx + W*0.5f, IY,     blitOffset, u1, v0);
		t.draw();
	}
}

void TouchWorldSelectionList::stepLeft() {
	if (selectedItem > 0) {
		int xoffset = (int)(xo - ((float)(selectedItem * itemWidth) + ((float)(itemWidth-width)) * 0.5f));
		td.start = xo;
		td.stop = xo - itemWidth - xoffset;
		td.cur = 0;
		td.dur = 8;
		mode = 1;
		tweenInited();
	}
}

void TouchWorldSelectionList::stepRight() {
	if (selectedItem >= 0 && selectedItem < getNumberOfItems()-1) {
		int xoffset = (int)(xo - ((float)(selectedItem * itemWidth) + ((float)(itemWidth-width)) * 0.5f));
		td.start = xo;
		td.stop = xo + itemWidth - xoffset;
		td.cur = 0;
		td.dur = 8;
		mode = 1;
		tweenInited();
	}
}

void TouchWorldSelectionList::commit() {
	for (unsigned int i = 0; i < levels.size(); ++i) {
		LevelSummary& level = levels[i];

		std::stringstream ss;
		ss << level.name << "/preview.png";
		TextureId id = Textures::InvalidId;//minecraft->textures->loadTexture(ss.str(), false);

		if (id != Textures::InvalidId) {
			_imageNames.push_back( ss.str() );
		} else {
			_imageNames.push_back("gui/default_world.png");
		}

		StringVector lines;
		lines.push_back(levels[i].name);
		lines.push_back(minecraft->platform()->getDateString(levels[i].lastPlayed));
		lines.push_back(levels[i].id);
		lines.push_back(LevelSettings::gameTypeToString(level.gameType));
		_descriptions.push_back(lines);

		selectedItem = 0;
	}
}

static float quadraticInOut(float t, float dur, float start, float stop) {
	const float delta = stop - start;
	const float T = (t / dur) * 2.0f;
	if (T < 1) return 0.5f*delta*T*T + start;
	return -0.5f*delta * ((T-1)*(T-3) - 1) + start;
}

void TouchWorldSelectionList::tick()
{
	RolledSelectionListH::tick();

	++currentTick;

	if (Mouse::isButtonDown(MouseAction::ACTION_LEFT) || dragState == 0)
		return;

	// Handle the tween (when in "mode 1")
	selectedItem = -1; 
	if (mode == 1) {
		if (++td.cur == td.dur) {
			mode = 0;
			xInertia = 0;
			xoo = xo = td.stop;
			selectedItem = getItemAtPosition(width/2, height/2);
		} else {
			tweenInited();
		}
		return;
	}

	// It's still going fast, let it run
	float speed = Mth::abs(xInertia);
	bool slowEnoughToBeBothered = speed < 5.0f;
	if (!slowEnoughToBeBothered) {
		xInertia = xInertia * .9f;
		return;
	}

	xInertia *= 0.8f;

	if (speed < 1 && dragState < 0) {
		const int offsetx = (width-itemWidth) / 2;
		const float pxo = xo + offsetx;
		int index = getItemAtXPositionRaw((int)(pxo - 10*xInertia));
		int indexPos = index*itemWidth;

		// Pick closest
		float diff = (float)indexPos - pxo;
		if (diff < -itemWidth/2) {
			diff += itemWidth;
			index++;
			//indexPos += itemWidth;
		}
		if (Mth::abs(diff) < 1 && speed < 0.1f) {
			selectedItem = getItemAtPosition(width/2, height/2);
			return;
		}

		td.start = xo;
		td.stop = xo + diff;
		td.cur = 0;
		td.dur = (float) Mth::Min(7, 1 + (int)(Mth::abs(diff) * 0.25f));
		mode = 1;
		//LOGI("inited-t %d\n", dragState);
		tweenInited();
	}
}

float TouchWorldSelectionList::getPos( float alpha )
{
	if (mode != 1) return RolledSelectionListH::getPos(alpha);

	float x0 = quadraticInOut(td.cur, td.dur, td.start, td.stop);
	float x1 = quadraticInOut(td.cur+1, td.dur, td.start, td.stop);
	return x0 + (x1-x0)*alpha;
}

bool TouchWorldSelectionList::capXPosition() {
	bool capped = RolledSelectionListH::capXPosition();
	if (capped) mode = 0;
	return capped;
}

void TouchWorldSelectionList::tweenInited() {
	float x0 = quadraticInOut(td.cur,   td.dur, td.start, td.stop);
	float x1 = quadraticInOut(td.cur+1, td.dur, td.start, td.stop);
	_xinertia = 0;
	xInertia = x0-x1; // yes, it's all backwards and messed up..
}

//
// Select World Screen
//
SelectWorldScreen::SelectWorldScreen()
:	bDelete (1, ""),
	bCreate (2, "Create new"),
	bBack   (3, "Back"),
	bHeader (0, "Select world"),
	bWorldView(4, ""),
	worldsList(NULL),
	_hasStartedLevel(false),
	_state(_STATE_DEFAULT)
{
	bDelete.active = false;

	// Delete button
	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;

	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	bDelete.setImageDef(def, true);

	// Create new, and Back button
/*
	def.src.y = 26; // @  0, 26
	def.src.w = def.w = 66; // 66, 26 size
	bBack.setImageDef(def, true);
	bCreate.setImageDef(def, true);
*/
}

SelectWorldScreen::~SelectWorldScreen()
{
	delete worldsList;
}

void SelectWorldScreen::init()
{
	worldsList = new TouchWorldSelectionList(minecraft, width, height);
	loadLevelSource();
	worldsList->commit();

	buttons.push_back(&bDelete);
	buttons.push_back(&bCreate);
	buttons.push_back(&bBack);
	buttons.push_back(&bHeader);

	_mouseHasBeenUp = !Mouse::getButtonState(MouseAction::ACTION_LEFT);

	tabButtons.push_back(&bWorldView);
	tabButtons.push_back(&bDelete);
	tabButtons.push_back(&bCreate);
	tabButtons.push_back(&bBack);
}

void SelectWorldScreen::setupPositions() {
	//#ifdef ANDROID
	bCreate.y =	0;
	bBack.y   = 0;
	bHeader.y = 0;
	bDelete.y = height - 30;

	// Center buttons
	bDelete.x   = (width - bDelete.width) / 2;
	bCreate.x   = width - bCreate.width;//width / 2					- bCreate.w / 2;
	bBack.x     = 0;//width / 2 + 4 + bCreate.w - bBack.w / 2;
	bHeader.x   = bBack.width;
	bHeader.width   = width - (bBack.width + bCreate.width);
	bHeader.height   = bCreate.height;
}

void SelectWorldScreen::buttonClicked(Button* button)
{
	if (button->id == bCreate.id) {
		if (_state == _STATE_DEFAULT && !_hasStartedLevel) {
			minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_CREATE_NEW_WORLD);
			_state = _STATE_CREATEWORLD;
		}
	}
	if (button->id == bDelete.id) {
		if (isIndexValid(worldsList->selectedItem)) {
			LevelSummary level = worldsList->levels[worldsList->selectedItem];
			LOGI("level: %s, %s\n", level.id.c_str(), level.name.c_str());
			minecraft->setScreen( new TouchDeleteWorldScreen(level) );
		}
	}
	if (button->id == bBack.id) {
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	}
	if (button->id == bWorldView.id) {
		// Try to "click" the item in the middle
		worldsList->selectItem( worldsList->getItemAtPosition(width/2, height/2), false );
	}
}

bool SelectWorldScreen::handleBackEvent(bool isDown)
{
	if (!isDown)
	{
		minecraft->cancelLocateMultiplayer();
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	}
	return true;
}

bool SelectWorldScreen::isIndexValid( int index )
{
	return worldsList && index >= 0 && index < worldsList->getNumberOfItems() - 1;
}

static char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

void SelectWorldScreen::tick()
{
	if (_state == _STATE_CREATEWORLD) {
		#if defined(RPI)
			std::string levelId = getUniqueLevelName("perf");
			//int seed = Util::hashCode("/r/Minecraft");
			LevelSettings settings(getEpochTimeS(), GameType::Creative);
			minecraft->selectLevel(levelId, levelId, settings);
			minecraft->hostMultiplayer();
			minecraft->setScreen(new ProgressScreen());
			_hasStartedLevel = true;
		#elif defined(WIN32)
			std::string name = getUniqueLevelName("perf");
			minecraft->setScreen(new SimpleChooseLevelScreen(name));
		#else
			int status = minecraft->platform()->getUserInputStatus();
            //LOGI("Status is: %d\n", status);
			if (status > -1) {
				if (status == 1) {
					StringVector sv = minecraft->platform()->getUserInput();
					
					// Read the level name.
					// 1) Trim name 2) Remove all bad chars 3) Append '-' chars 'til the name is unique
					std::string levelName = Util::stringTrim(sv[0]);
					std::string levelId = levelName;

					for (int i = 0; i < sizeof(ILLEGAL_FILE_CHARACTERS) / sizeof(char); ++i)
						levelId = Util::stringReplace(levelId, std::string(1, ILLEGAL_FILE_CHARACTERS[i]), "");
                    if ((int)levelId.length() == 0) {
                        levelId = "no_name";
                    }
					levelId = getUniqueLevelName(levelId);

					// Read the seed
					int seed = getEpochTimeS();
					if (sv.size() >= 2) {
						std::string seedString = Util::stringTrim(sv[1]);
						if (seedString.length() > 0) {
							int tmpSeed;
							// Try to read it as an integer
							if (sscanf(seedString.c_str(), "%d", &tmpSeed) > 0) {
								seed = tmpSeed;
							} // Hash the "seed"
							else {
								seed = Util::hashCode(seedString);
							}
						}
					}
					// Read the game mode
					bool isCreative = true;
					if (sv.size() >= 3 && sv[2] == "survival")
						isCreative = false;

					// Start a new level with the given name and seed
					LOGI("Creating a level with id '%s', name '%s' and seed '%d'\n", levelId.c_str(), levelName.c_str(), seed);
					LevelSettings settings(seed, isCreative? GameType::Creative : GameType::Survival);
					minecraft->selectLevel(levelId, levelName, settings);
					minecraft->hostMultiplayer();
					minecraft->setScreen(new ProgressScreen());
					_hasStartedLevel = true;
				}
				_state = _STATE_DEFAULT;
				// Reset the world list
				worldsList->hasPickedLevel = false;
				worldsList->pickedIndex = -1;
			}
		#endif

		worldsList->hasPickedLevel = false;
		return;
	}

	worldsList->tick();

	if (worldsList->hasPickedLevel) {
		if (worldsList->pickedIndex == worldsList->levels.size()) {
			worldsList->hasPickedLevel = false;
			minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_CREATE_NEW_WORLD);
			_state = _STATE_CREATEWORLD;
		} else {
			minecraft->selectLevel(worldsList->pickedLevel.id, worldsList->pickedLevel.name, LevelSettings::None());
			minecraft->hostMultiplayer();
			minecraft->setScreen(new ProgressScreen());
			_hasStartedLevel = true;
			return;
		}
	}

	// copy the currently selected item
	LevelSummary selectedWorld;
	//bool hasSelection = false;
	if (isIndexValid(worldsList->selectedItem))
	{
		selectedWorld = worldsList->levels[worldsList->selectedItem];
		//hasSelection = true;
	}

	bDelete.active = isIndexValid(worldsList->selectedItem);
}

void SelectWorldScreen::render( int xm, int ym, float a )
{
	//Performance::watches.get("sws-full").start();
	//Performance::watches.get("sws-renderbg").start();
	renderBackground();
	//Performance::watches.get("sws-renderbg").stop();
	//Performance::watches.get("sws-worlds").start();

	worldsList->setComponentSelected(bWorldView.selected);

	if (_mouseHasBeenUp)
		worldsList->render(xm, ym, a);
	else {
		worldsList->render(0, 0, a);
		_mouseHasBeenUp = !Mouse::getButtonState(MouseAction::ACTION_LEFT);
	}

	//Performance::watches.get("sws-worlds").stop();
	//Performance::watches.get("sws-screen").start();
	Screen::render(xm, ym, a);
	//Performance::watches.get("sws-screen").stop();

	//minecraft->textures->loadAndBindTexture("gui/selectworld/trash.png");

	//Performance::watches.get("sws-string").start();
	//Performance::watches.get("sws-string").stop();

	//Performance::watches.get("sws-full").stop();
	//Performance::watches.printEvery(128);
}

void SelectWorldScreen::loadLevelSource()
{
	LevelStorageSource* levelSource = minecraft->getLevelSource();
	levelSource->getLevelList(levels);
	std::sort(levels.begin(), levels.end());

	for (unsigned int i = 0; i < levels.size(); ++i) {
		if (levels[i].id != LevelStorageSource::TempLevelId)
			worldsList->levels.push_back( levels[i] );
	}
}


std::string SelectWorldScreen::getUniqueLevelName( const std::string& level )
{
	std::set<std::string> Set;
	for (unsigned int i = 0; i < levels.size(); ++i)
		Set.insert(levels[i].id);

	std::string s = level;
	while ( Set.find(s) != Set.end() )
		s += "-";
	return s;
}

bool SelectWorldScreen::isInGameScreen() { return true;  }

void SelectWorldScreen::keyPressed( int eventKey )
{
	if (bWorldView.selected) {
		if (eventKey == minecraft->options.keyLeft.key)
			worldsList->stepLeft();
		if (eventKey == minecraft->options.keyRight.key)
			worldsList->stepRight();
	}

	Screen::keyPressed(eventKey);
}

//
// Delete World Screen
//
TouchDeleteWorldScreen::TouchDeleteWorldScreen(const LevelSummary& level)
:	ConfirmScreen(NULL, "Are you sure you want to delete this world?",
						"'" + level.name + "' will be lost forever!",
						"Delete", "Cancel", 0),
	_level(level)
{
	tabButtonIndex = 1;
}

void TouchDeleteWorldScreen::postResult( bool isOk )
{
	if (isOk) {
		LevelStorageSource* storageSource = minecraft->getLevelSource();
		storageSource->deleteLevel(_level.id);
	}
	minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
}

};
