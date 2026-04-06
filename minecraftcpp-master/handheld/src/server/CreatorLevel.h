#ifndef NET_MINECRAFT_WORLD_LEVEL__CreatorLevel_H__
#define NET_MINECRAFT_WORLD_LEVEL__CreatorLevel_H__

#include "../world/level/Level.h"

class CreatorLevel: public Level
{
	typedef Level super;
public:
	CreatorLevel(LevelStorage* levelStorage, const std::string& levelName, const LevelSettings& settings, int generatorVersion, Dimension* fixedDimension = NULL);

	void tick();
};

#endif /* NET_MINECRAFT_WORLD_LEVEL__CreatorLevel_H__ */
