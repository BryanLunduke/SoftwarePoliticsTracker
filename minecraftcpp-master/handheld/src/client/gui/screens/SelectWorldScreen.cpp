#include "SelectWorldScreen.h"
#include "StartMenuScreen.h"
#include "ProgressScreen.h"
#include "DialogDefinitions.h"
#include "../../renderer/Tesselator.h"
#include "../../../AppPlatform.h"
#include "../../../util/StringUtils.h"
#include "../../../util/Mth.h"
#include "../../../platform/input/Mouse.h"
#include "../../../Performance.h"
#include "../../../world/level/LevelSettings.h"

#include <algorithm>
#include <set>
#include "../../renderer/Textures.h"
#include "SimpleChooseLevelScreen.h"

static float Max(float a, float b) {
	return a>b? a : b;
}

//
// World Selection List
//
WorldSelectionList::WorldSelectionList( Minecraft* minecraft, int width, int height )
:	_height(height),
	hasPickedLevel(false),
	currentTick(0),
	stoppedTick(-1),
	mode(0),
	RolledSelectionListH(minecraft, width, height, 0, width, 26, height-32, 120)
{
}

int WorldSelectionList::getNumberOfItems() {
	return (int)levels.size();
}

void WorldSelectionList::selectItem( int item, bool doubleClick ) {
	//LOGI("sel: %d, item %d\n", selectedItem, item);
	if (selectedItem < 0 || (selectedItem != item))
		return;
	
	if (!hasPickedLevel) {
		hasPickedLevel = true;
		pickedLevel = levels[item];
	}
}

bool WorldSelectionList::isSelectedItem( int item ) {
	return item == selectedItem;
}

void WorldSelectionList::renderItem( int i, int x, int y, int h, Tesselator& t ) {

	int centerx = x + itemWidth/2;

	float a0 = Max(1.1f - std::abs( width / 2 - centerx ) * 0.0055f, 0.2f);
	if (a0 > 1) a0 = 1;
	int textColor =  (int)(255.0f * a0) * 0x010101;
	int textColor2 = (int)(140.0f * a0) * 0x010101;

	const int TY = y + 42;
	const int TX = centerx - itemWidth / 2 + 5;

	StringVector v = _descriptions[i];
	drawString(minecraft->font, v[0].c_str(), TX, TY +  0, textColor);
	drawString(minecraft->font, v[1].c_str(), TX, TY + 10, textColor2);
	drawString(minecraft->font, v[2].c_str(), TX, TY + 20, textColor2);
	drawString(minecraft->font, v[3].c_str(), TX, TY + 30, textColor2);

	minecraft->textures->loadAndBindTexture(_imageNames[i]);
	t.color(0.3f, 1.0f, 0.2f);

	//float x0 = (float)x;
	//float x1 = (float)x + (float)itemWidth;

	const float IY = (float)y - 8;
	t.begin();
		t.color(textColor);
		t.vertexUV((float)(centerx-32), IY,      blitOffset, 0, 0);
		t.vertexUV((float)(centerx-32), IY + 48, blitOffset, 0, 1);
		t.vertexUV((float)(centerx+32), IY + 48, blitOffset, 1, 1);
		t.vertexUV((float)(centerx+32), IY,      blitOffset, 1, 0);
	t.draw();
}

void WorldSelectionList::stepLeft() {
	if (selectedItem > 0) {
		td.start = xo;
		td.stop = xo - itemWidth;
		td.cur = 0;
		td.dur = 8;
		mode = 1;
		tweenInited();
	}
}

void WorldSelectionList::stepRight() {
	if (selectedItem >= 0 && selectedItem < getNumberOfItems()-1) {
		td.start = xo;
		td.stop = xo + itemWidth;
		td.cur = 0;
		td.dur = 8;
		mode = 1;
		tweenInited();
	}
}

void WorldSelectionList::commit() {
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
		lines.push_back(level.name);
		lines.push_back(minecraft->platform()->getDateString(level.lastPlayed));
		lines.push_back(level.id);
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

void WorldSelectionList::tick()
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

float WorldSelectionList::getPos( float alpha )
{
	if (mode != 1) return RolledSelectionListH::getPos(alpha);

	float x0 = quadraticInOut(td.cur, td.dur, td.start, td.stop);
	float x1 = quadraticInOut(td.cur+1, td.dur, td.start, td.stop);
	return x0 + (x1-x0)*alpha;
}

bool WorldSelectionList::capXPosition() {
	bool capped = RolledSelectionListH::capXPosition();
	if (capped) mode = 0;
	return capped;
}

void WorldSelectionList::tweenInited() {
	float x0 = quadraticInOut(td.cur,   td.dur, td.start, td.stop);
	float x1 = quadraticInOut(td.cur+1, td.dur, td.start, td.stop);
	xInertia = x0-x1; // yes, it's all backwards and messed up..
}

//
// Select World Screen
//
SelectWorldScreen::SelectWorldScreen()
:	bDelete (1, "Delete"),
	bCreate (2, "Create new"),
	bBack   (3, "Back"),
	bWorldView(4, ""),
	worldsList(NULL),
	_state(_STATE_DEFAULT),
	_hasStartedLevel(false)
{
	bDelete.active = false;
}

SelectWorldScreen::~SelectWorldScreen()
{
	delete worldsList;
}

void SelectWorldScreen::buttonClicked(Button* button)
{
	if (button->id == bCreate.id) {
		//minecraft->setScreen( new CreateWorldScreen() );
		//minecraft->locateMultiplayer();
		//minecraft->setScreen(new JoinGameScreen());

		//minecraft->hostMultiplayer();
		//minecraft->setScreen(new ProgressScreen());

		if (_state == _STATE_DEFAULT && !_hasStartedLevel) {
			minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_CREATE_NEW_WORLD);
			_state = _STATE_CREATEWORLD;
		}
	}
	if (button->id == bDelete.id) {
		if (isIndexValid(worldsList->selectedItem)) {
			LevelSummary level = worldsList->levels[worldsList->selectedItem];
			LOGI("level: %s, %s\n", level.id.c_str(), level.name.c_str());
			minecraft->setScreen( new DeleteWorldScreen(level) );
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
	return worldsList && index >= 0 && index < worldsList->getNumberOfItems();
}

static char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

void SelectWorldScreen::tick()
{
	if (_state == _STATE_CREATEWORLD) {
		#if defined(RPI)
			std::string levelId = getUniqueLevelName("world");
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
					LevelSettings settings(seed, isCreative? GameType::Creative : GameType::Survival);
					LOGI("Creating a level with id '%s', name '%s' and seed '%d'\n", levelId.c_str(), levelName.c_str(), seed);
					minecraft->selectLevel(levelId, levelName, settings);
					minecraft->hostMultiplayer();
					minecraft->setScreen(new ProgressScreen());
					_hasStartedLevel = true;
				}
				_state = _STATE_DEFAULT;
			}
		#endif

		return;
	}

	worldsList->tick();

	if (worldsList->hasPickedLevel) {
		minecraft->selectLevel(worldsList->pickedLevel.id, worldsList->pickedLevel.name, LevelSettings::None());
		minecraft->hostMultiplayer();
		minecraft->setScreen(new ProgressScreen());
		_hasStartedLevel = true;
		return;
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

void SelectWorldScreen::init()
{
	worldsList = new WorldSelectionList(minecraft, width, height);
	loadLevelSource();
	worldsList->commit();

	buttons.push_back(&bDelete);
	buttons.push_back(&bCreate);
	buttons.push_back(&bBack);

	_mouseHasBeenUp = !Mouse::getButtonState(MouseAction::ACTION_LEFT);

	tabButtons.push_back(&bWorldView);
	tabButtons.push_back(&bDelete);
	tabButtons.push_back(&bCreate);
	tabButtons.push_back(&bBack);
}

void SelectWorldScreen::setupPositions() {
	int yBase = height - 28;

	//#ifdef ANDROID
	bCreate.y =	yBase;
	bBack.y   = yBase;
	bDelete.y = yBase;

	bBack.width = bDelete.width = bCreate.width = 84;
	//bDelete.h = bCreate.h = bBack.h = 24;
	//#endif

	// Center buttons
	bDelete.x   = width / 2 - 4 - bDelete.width - bDelete.width / 2;
	bCreate.x   = width / 2					- bCreate.width / 2;
	bBack.x     = width / 2 + 4 + bCreate.width - bBack.width / 2;
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

	//Performance::watches.get("sws-string").start();
	drawCenteredString(minecraft->font, "Select world", width / 2, 8, 0xffffffff);
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
DeleteWorldScreen::DeleteWorldScreen(const LevelSummary& level)
:	ConfirmScreen(NULL, "Are you sure you want to delete this world?",
						"'" + level.name + "' will be lost forever!",
						"Delete", "Cancel", 0),
	_level(level)
{
	tabButtonIndex = 1;
}

void DeleteWorldScreen::postResult( bool isOk )
{
	if (isOk) {
		LevelStorageSource* storageSource = minecraft->getLevelSource();
		storageSource->deleteLevel(_level.id);
	}
	minecraft->screenChooser.setScreen(SCREEN_SELECTWORLD);
}
