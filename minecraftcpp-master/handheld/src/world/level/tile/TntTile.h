#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__TntTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__TntTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"

#include "../../entity/item/PrimedTnt.h"
#include "../Level.h"
#include "../material/Material.h"

class TntTile: public Tile
{
	typedef Tile super;
	static const int EXPLODE_BIT = 1;
public:
	TntTile(int id, int tex)
	:	super(id, tex, Material::explosive)
	{
    }

    int getTexture(int face) {
        if (face == 0) return tex + 2;
        if (face == 1) return tex + 1;
        return tex;
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        if (type > 0 && Tile::tiles[type]->isSignalSource()) {
            if (level->hasNeighborSignal(x, y, z)) {
                destroy(level, x, y, z, EXPLODE_BIT);
                level->setTile(x, y, z, 0);
            }
        }
    }

    int getResourceCount(Random* random) {
        return 1;
    }

    void wasExploded(Level* level, int x, int y, int z) {
        PrimedTnt* primed = new PrimedTnt(level, x + 0.5f, y + 0.5f, z + 0.5f);
        primed->life = level->random.nextInt(primed->life / 4) + primed->life / 8;
        level->addEntity(primed);
    }

    void destroy(Level* level, int x, int y, int z, int data) {
        if (level->isClientSide) return;

		if ((data & EXPLODE_BIT) == 1) {
			PrimedTnt* tnt = new PrimedTnt(level, x + 0.5f, y + 0.5f, z + 0.5f);
			level->addEntity(tnt);
			level->playSound(tnt, "random.fuse", 1, 1.0f);
		}
    }
	
	bool use(Level* level, int x, int y, int z, Player* player) {
		ItemInstance* carried = player->getSelectedItem();
		if (carried && carried->id == Item::flintAndSteel->id) {
			carried->hurt(1);
			destroy(level, x, y, z, EXPLODE_BIT);
			level->setTile(x, y, z, 0);
			return true;
		}
		return super::use(level, x, y, z, player);
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__TntTile_H__*/
