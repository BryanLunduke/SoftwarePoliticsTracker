#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__RedStoneOreTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__RedStoneOreTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

class Player;

class RedStoneOreTile: public Tile
{
    bool lit;

public:
	RedStoneOreTile(int id, int tex, bool lit)
	:	Tile(id, tex, Material::stone)
	{
        if (lit) {
            this->setTicking(true);
        }
        this->lit = lit;
    }

    int getTickDelay() {
        return 30;
    }

    void attack(Level* level, int x, int y, int z, Player* player) {
        interact(level, x, y, z);
        Tile::attack(level, x, y, z, player);
    }

    void stepOn(Level* level, int x, int y, int z, Entity* entity) {
        interact(level, x, y, z);
        Tile::stepOn(level, x, y, z, entity);
    }

    bool use(Level* level, int x, int y, int z, Player* player) {
        interact(level, x, y, z);
        return Tile::use(level, x, y, z, player);
    }

    /*private*/ void interact(Level* level, int x, int y, int z) {
        poofParticles(level, x, y, z);
        if (id == Tile::redStoneOre->id) {
            level->setTile(x, y, z, Tile::redStoneOre_lit->id);
        }
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (id == Tile::redStoneOre_lit->id) {
            level->setTile(x, y, z, Tile::redStoneOre->id);
        }
    }

    int getResource(int data, Random* random) {
        //return Item.redStone.id;
		return 0;
    }

    int getResourceCount(Random* random) {
        return 4 + random->nextInt(2);
    }

    void animateTick(Level* level, int x, int y, int z, Random* random) {
        if (lit) {
            poofParticles(level, x, y, z);
        }
    }
private:
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
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__RedStoneOreTile_H__*/
