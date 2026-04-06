#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TallgrassFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TallgrassFeature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "Feature.h"

class TallgrassFeature : public Feature
{
	typedef Feature super;
public:
	TallgrassFeature(int tile, int type)
	:	super(false), tile(tile), type(type)
	{
	}

	bool place(Level* level, Random* random, int x, int y, int z) {
       int t = 0;
	   while (((t = level->getTile(x, y, z)) == 0 || t == Tile::leaves->id) && y > 0)
		   y--;

	   for (int i = 0; i < 128; i++) {
		   int x2 = x + random->nextInt(8) - random->nextInt(8);
		   int y2 = y + random->nextInt(4) - random->nextInt(4);
		   int z2 = z + random->nextInt(8) - random->nextInt(8);
		   if (level->isEmptyTile(x2, y2, z2)) {
			   if (Tile::tiles[tile]->canSurvive(level, x2, y2, z2)) {
				   level->setTileAndDataNoUpdate(x2, y2, z2, tile, type);
			   }
		   }
	   }

	   return true;
    }
private:
	int tile;
	int type;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__TallgrassFeature_H__*/
