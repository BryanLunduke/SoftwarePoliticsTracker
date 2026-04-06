#include "ChooseLevelScreen.h"
#include <algorithm>
#include <set>
#include "../../Minecraft.h"

void ChooseLevelScreen::init() {
	loadLevelSource();
}

void ChooseLevelScreen::loadLevelSource()
{
	LevelStorageSource* levelSource = minecraft->getLevelSource();
	levelSource->getLevelList(levels);
	std::sort(levels.begin(), levels.end());
}

std::string ChooseLevelScreen::getUniqueLevelName( const std::string& level ) {
	std::set<std::string> Set;
	for (unsigned int i = 0; i < levels.size(); ++i)
		Set.insert(levels[i].id);

	std::string s = level;
	while ( Set.find(s) != Set.end() )
		s += "-";
	return s;
}
