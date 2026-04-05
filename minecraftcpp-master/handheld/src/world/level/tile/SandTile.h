#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__SandTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__SandTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"

class Level;

class SandTile: public Tile
{
public:
    static bool instaFall;

    SandTile(int type, int tex)
	:	Tile(type, tex, Material::sand)
	{
    }

	// id == 0 -> not possible to create via serialization (yet)
	int getEntityTypeId() const { return 0; } // @todo

    void onPlace(Level* level, int x, int y, int z) {
        //level->addToTickNextTick(x, y, z, id, getTickDelay());
    }

    void neighborChanged(Level* level, int x, int y, int z, int type) {
        //level->addToTickNextTick(x, y, z, id, getTickDelay());
    }

    void tick(Level* level, int x, int y, int z, Random* random) {
		checkSlide(level, x, y, z);
    }

    int getTickDelay() {
        return 3;
    }

    static bool isFree(Level* level, int x, int y, int z) {
        int t = level->getTile(x, y, z);
        if (t == 0) return true;
        if (t == ((Tile*)Tile::fire)->id) return true;
        const Material* material = Tile::tiles[t]->material;
        if (material == Material::water) return true;
        if (material == Material::lava) return true;
        return false;
    }

private:
    void checkSlide(Level* level, int x, int y, int z);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__SandTile_H__*/
