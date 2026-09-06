#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__StonecutterTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__StonecutterTile_H__

#include "Tile.h"
#include "../Level.h"
#include "../material/Material.h"
#include "../../entity/player/Player.h"

class StonecutterTile: public Tile
{
	typedef Tile super;
public:
    StonecutterTile(int id)
	:	super(id, Material::stone)
	{
        tex = 13 + 16 * 2;
    }

    int getTexture(int face) {
		if (face == Facing::UP) return 9 + 10 * 16;
		if (face == Facing::DOWN) return 14 + 3 * 16;
		if (face == Facing::NORTH || face == Facing::SOUTH) return 8 + 10 * 16;
		return tex;
    }

	bool use(Level* level, int x, int y, int z, Player* player) {
		player->startStonecutting(x, y, z);
        return true;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__StonecutterTile_H__*/
