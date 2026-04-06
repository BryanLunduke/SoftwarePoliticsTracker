#include "Feature.h"

Feature::Feature( bool doUpdate /*= false*/ )
:	doUpdate(doUpdate)
{
}

void Feature::placeBlock( Level* level, int x, int y, int z, int tile )
{
	placeBlock(level, x, y, z, tile, 0);
}

void Feature::placeBlock( Level* level, int x, int y, int z, int tile, int data )
{
	if (doUpdate) {
		level->setTileAndData(x, y, z, tile, data);
		/*
		} else if (level->hasChunkAt(x, y, z) && level->getChunkAt(x, z).seenByPlayer) {
		if (level->setTileAndDataNoUpdate(x, y, z, tile, data)) {
		level->sendTileUpdated(x, y, z);
		}
		*/
	} else {
		level->setTileAndDataNoUpdate(x, y, z, tile, data);
	}
}
