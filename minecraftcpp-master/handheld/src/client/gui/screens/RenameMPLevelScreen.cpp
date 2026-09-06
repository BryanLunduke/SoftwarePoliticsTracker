#include "RenameMPLevelScreen.h"
#include "StartMenuScreen.h"
#include "DialogDefinitions.h"
#include "../Gui.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "../../../platform/log.h"
#include "../../../world/level/storage/LevelStorageSource.h"


static char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

RenameMPLevelScreen::RenameMPLevelScreen( const std::string& levelId )
:	_levelId(levelId)
{
}

void RenameMPLevelScreen::init() {
	minecraft->platform()->createUserInput(DialogDefinitions::DIALOG_RENAME_MP_WORLD);
}

void RenameMPLevelScreen::render(int xm, int ym, float a)
{
	renderBackground();

	#ifdef WIN32
		minecraft->getLevelSource()->renameLevel(_levelId, "Save?Level");
		minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	#else
		int status = minecraft->platform()->getUserInputStatus();
		if (status > -1) {
			if (status == 1) {
				std::vector<std::string> v = minecraft->platform()->getUserInput();

				if (!v.empty()) {
                    // Read the level name.
					// 1) Trim name 2) Remove all bad chars -) We don't have to getUniqueLevelName, since renameLevel will do that
					std::string levelId = v[0];

					for (int i = 0; i < sizeof(ILLEGAL_FILE_CHARACTERS) / sizeof(char); ++i)
						levelId = Util::stringReplace(levelId, std::string(1, ILLEGAL_FILE_CHARACTERS[i]), "");
                    if ((int)levelId.length() == 0) {
                        levelId = "saved_world";
                    }

					minecraft->getLevelSource()->renameLevel(_levelId, levelId);
                }
			}

			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
	#endif
}
