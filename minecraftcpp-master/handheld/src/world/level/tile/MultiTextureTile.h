#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__MultiTextureTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__MultiTextureTile_H__

//package net.minecraft.world.level.tile;

#include "../material/Material.h"

class MultiTextureTile: public Tile
{
	typedef Tile super;
public:
    MultiTextureTile(int id, const int* textures, int texCount, const Material* material)
	:	super(id, textures[0], material),
		_textures(textures),
		_textureCount(texCount)
	{
	}

    virtual int getTexture(int face, int data) {
		if (data >= 0 && data < _textureCount)
		{
			return _textures[data];
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
	static int getItemAuxValueForTileData(int data) {
		return data;
	}

	const int* _textures;
	int _textureCount;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__ClothTile_H__*/
