#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileStatic_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileStatic_H__

//package net.minecraft.world.level.tile;

#include "LiquidTile.h"
#include "../Level.h"
#include "FireTile.h"

class LiquidTileStatic: public LiquidTile
{
	typedef LiquidTile super;
public:
    LiquidTileStatic(int id, const Material* material)
	:	super(id, material)
	{
        setTicking(false);
        if (material == Material::lava) setTicking(true);
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        super::neighborChanged(level, x, y, z, type);
        if (level->getTile(x, y, z) == id) {
            setDynamic(level, x, y, z);
        }
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
		//@fire
		return;

		if (material == Material::lava) {
            int h = random->nextInt(3);
            for (int i = 0; i < h; i++) {
                x += random->nextInt(3) - 1;
                y++;
                z += random->nextInt(3) - 1;
                int t = level->getTile(x, y, z);
                if (t == 0) {
                    if (isFlammable(level, x - 1, y, z) || isFlammable(level, x + 1, y, z) || isFlammable(level, x, y, z - 1) || isFlammable(level, x, y, z + 1) || isFlammable(level, x, y - 1, z) || isFlammable(level, x, y + 1, z)) {
                        level->setTile(x, y, z, Tile::fire->id);
                        return;
                    }
                } else if (Tile::tiles[t]->material->blocksMotion()) {
                    return;
                }

            }
        }
    }
private:
    bool isFlammable(Level* level, int x, int y, int z) {
        return level->getMaterial(x, y, z)->isFlammable();
    }

	void setDynamic(Level* level, int x, int y, int z) {
        int d = level->getData(x, y, z);
        level->noNeighborUpdate = true;
        level->setTileAndDataNoUpdate(x, y, z, id - 1, d);
        level->setTilesDirty(x, y, z, x, y, z);
        level->addToTickNextTick(x, y, z, id - 1, getTickDelay());
        level->noNeighborUpdate = false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__LiquidTileStatic_H__*/
