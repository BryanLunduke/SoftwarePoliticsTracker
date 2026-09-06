#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__QuartzBlockTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__QuartzBlockTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../../Facing.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class QuartzBlockTile: public Tile
{
	typedef Tile super;
public:

	static const int TYPE_DEFAULT = 0;
	static const int TYPE_CHISELED  = 1;
	static const int TYPE_LINES  = 2;

	QuartzBlockTile(int id)
	:	super(id, 4 + 13 * 16, Material::stone)
	{}

	int getTexture(int face, int data) {
		if (face == Facing::UP || face == Facing::DOWN) {
			if (data == TYPE_CHISELED)
			{
				return 6 + 12 * 16;
			}
			if (data == TYPE_LINES)
			{
				return 5 + 12 * 16;
			}
			if (face == Facing::DOWN) {
				return 3 + 13 * 16;
			}
			return 4 + 12 * 16;
		}
		if (data == TYPE_CHISELED)
		{
			return 6 + 13 * 16;
		}
		if (data == TYPE_LINES)
		{
			return 5 + 13 * 16;
		}
		return tex;
	}

	static int getTileDataForItemAuxValue(int auxValue) {
		return (auxValue & 0xf);
	}

protected:

	int getSpawnResourcesAuxValue(int data) {
		return data;
	}

};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__QuartzBlockTile_H__*/
