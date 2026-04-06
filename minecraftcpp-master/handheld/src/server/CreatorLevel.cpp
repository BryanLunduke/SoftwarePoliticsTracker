#include "CreatorLevel.h"
#include "../world/level/chunk/ChunkSource.h"
#include "../util/PerfTimer.h"

CreatorLevel::CreatorLevel(LevelStorage* levelStorage, const std::string& levelName, const LevelSettings& settings, int generatorVersion, Dimension* fixedDimension /* = NULL */)
:	super(levelStorage, levelName, settings, generatorVersion, fixedDimension)
{
}

void CreatorLevel::tick() {
	TIMER_PUSH("chunkSource");
	_chunkSource->tick();

	long time = levelData.getTime() + 1;
	levelData.setTime(time);

	TIMER_POP_PUSH("tickPending");
	tickPendingTicks(false);

	TIMER_POP();
}

/*
void CreatorLevel::addToTickNextTick(int x, int y, int z, int tileId, int tickDelay) {
	if (tileId >= Tile::water->id && tileId <= calmLava)
		super::addToTickNextTick(x, y, z, tileId, tickDelay);
}
*/
