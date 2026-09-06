#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__InvisibleTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__InvisibleTile_H__

#include "Tile.h"

class InvisibleTile : public Tile
{
	typedef Tile super;
public:
	InvisibleTile(int id, int tex, const Material* material)
	:	super(id, tex, material)
	{
	}

	virtual int getRenderShape()
	{
		return SHAPE_INVISIBLE;
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__InvisibleTile_H__*/
