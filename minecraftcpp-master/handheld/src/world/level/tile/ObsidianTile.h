#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__ObsidianTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__ObsidianTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "StoneTile.h"
class

 ObsidianTile: public StoneTile
{
public:
    ObsidianTile(int id, int tex, bool isGlowing)
	:	StoneTile(id, tex), isGlowing(isGlowing)
	{
    }
    int getResourceCount(Random* random) {
        return 1;
    }

    int getResource(int data, Random* random) {
        return Tile::obsidian->id;
    }
	void animateTick(Level* level, int x, int y, int z, Random* random) {
		if(isGlowing) {
			poofParticles(level, x, y, z);
		}
	}
	void poofParticles(Level* level, int x, int y, int z) {
		Random& random = level->random;
		float r = 1 / 16.0f;
		for (int i = 0; i < 6; i++) {
			float xx = x + random.nextFloat();
			float yy = y + random.nextFloat();
			float zz = z + random.nextFloat();
			if (i == 0 && !level->isSolidBlockingTile(x, y + 1, z)) yy = y + 1 + r;
			if (i == 1 && !level->isSolidBlockingTile(x, y - 1, z)) yy = y + 0 - r;
			if (i == 2 && !level->isSolidBlockingTile(x, y, z + 1)) zz = z + 1 + r;
			if (i == 3 && !level->isSolidBlockingTile(x, y, z - 1)) zz = z + 0 - r;
			if (i == 4 && !level->isSolidBlockingTile(x + 1, y, z)) xx = x + 1 + r;
			if (i == 5 && !level->isSolidBlockingTile(x - 1, y, z)) xx = x + 0 - r;
			if (xx < x || xx > x + 1 || yy < 0 || yy > y + 1 || zz < z || zz > z + 1) {
				level->addParticle(PARTICLETYPE(reddust), xx, yy, zz, 0, 0, 0);
			}
		}
	}
private:
	bool isGlowing;
/*    void wasExploded(Level* level, int x, int y, int z) {
        float s = 0.7f;
        float xo = level->random.nextFloat() * s + (1 - s) * 0.5;
        float yo = level->random.nextFloat() * s + (1 - s) * 0.5;
        float zo = level->random.nextFloat() * s + (1 - s) * 0.5;
        ItemEntity* item = new ItemEntity(level, x + xo, y + yo, z + zo, new ItemInstance(id));
        item->throwTime = 10;
        level->addEntity(item);
    }*/
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__ObsidianTile_H__*/
