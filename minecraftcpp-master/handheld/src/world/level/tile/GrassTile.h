#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__GrassTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__GrassTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"
#include "../LevelSource.h"

#include "Tile.h"

class GrassTile: public Tile
{
	typedef Tile super;
public:
	static const int MIN_BRIGHTNESS = 4;

	GrassTile(int id);

	int getTexture(LevelSource* level, int x, int y, int z, int face);
	int getTexture(int face, int data);
    int getColor(LevelSource* level, int x, int y, int z);

    void tick(Level* level, int x, int y, int z, Random* random);
    int getResource(int data, Random* random);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__GrassTile_H__*/
