#include "NetherReactorPattern.h"
#include "Tile.h"
NetherReactorPattern::NetherReactorPattern( ) {
	const int goldId = Tile::goldBlock->id;
	const int stoneId = Tile::stoneBrick->id;
	const int netherCoreId = Tile::netherReactor->id;
	const unsigned int types[3][3][3] =
	{
		// Level 0
		{
			{goldId, stoneId, goldId},
			{stoneId, stoneId, stoneId},
			{goldId, stoneId, goldId}
		},
			// Level 1
		{
			{stoneId, 0, stoneId},
			{0, netherCoreId, 0},
			{stoneId, 0, stoneId}
		},
			// Level 2
		{
			{0, stoneId, 0},
			{stoneId, stoneId, stoneId},
			{0, stoneId, 0}
		}
	};
	for(int setLevel = 0; setLevel <= 2; ++setLevel) {
		for(int setX = 0; setX <= 2; ++setX) {
			for(int setZ = 0; setZ <= 2; ++setZ) {
				setTileAt(setLevel, setX, setZ, types[setLevel][setX][setZ]);
			}
		}
	}
}

void NetherReactorPattern::setTileAt( int level, int x, int z, int tile) {
	pattern[level][x][z] = tile;
}

unsigned int NetherReactorPattern::getTileAt( int level, int x, int z ) {
	return pattern[level][x][z];
}
