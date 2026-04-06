#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__ClothTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__ClothTile_H__

//package net.minecraft.world.level.tile;

#include "../material/Material.h"

class ClothTile: public Tile
{
	typedef Tile super;
	static const int COLOR_TEX_POS = 7 * 16 + 1;
public:
    ClothTile(int id)
	:	super(id, 4 * 16, Material::cloth)
	{
	}
	ClothTile(int id, int data)
		:	super(id, 4 * 16, Material::cloth)
	{
	}

    int getTexture(int face, int data) {
        if (data == 0) {
            // un-colored cloth
            return tex;
        }

        // invert bits (so 1111 becomes black)
        data = ~(data & 0xf);
        return COLOR_TEX_POS + ((data & 8) >> 3) + ((data & 7) * 16);
    }

	static int getTileDataForItemAuxValue(int auxValue) {
		return (~auxValue & 0xf);
	}
protected:
    int getSpawnResourcesAuxValue(int data) {
        return data;
    }
	static int getItemAuxValueForTileData(int data) {
		return (~data & 0xf);
	}
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__ClothTile_H__*/
