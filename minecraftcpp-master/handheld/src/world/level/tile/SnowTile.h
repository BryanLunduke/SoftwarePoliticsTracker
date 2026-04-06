#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__SnowTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__SnowTile_H__

//package net.minecraft.world.level->tile;

#include "Tile.h"

#include "../material/Material.h"
#include "../../item/Item.h"
#include "../../../util/Random.h"

class SnowTile: public Tile
{
	typedef Tile super;
public:
    SnowTile(int id, int tex)
    :   super(id, tex, Material::snow)
    {
        setTicking(true);
    }

	int getResource(int data, Random* random) {
        return Item::snowBall->id;
    }

    int getResourceCount(Random* random) {
        return 4;
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
        if (level->getBrightness(LightLayer::Block, x, y, z) > 11) {
            this->spawnResources(level, x, y, z, level->getData(x, y, z), 0);
            level->setTile(x, y, z, 0);
        }
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__SnowTile_H__*/
