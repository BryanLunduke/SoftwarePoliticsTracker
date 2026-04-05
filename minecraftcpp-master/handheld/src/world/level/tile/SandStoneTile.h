#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__SandStoneTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__SandStoneTile_H__

//package net.minecraft.world.level.tile;

#include "../../../util/Random.h"
#include "../../Facing.h"
#include "../material/Material.h"
#include "../Level.h"

#include "Tile.h"
#include "MultiTextureTile.h"

class SandStoneTile: public MultiTextureTile
{
	typedef MultiTextureTile super;
public:
	SandStoneTile(int id, const int* textures, int texCount)
	:	super(id, textures, texCount, Material::stone)
	{}

	int getTexture(int face, int data) {
		if (face == Facing::UP || (face == Facing::DOWN && data > 0)) {
			return 11 * 16;//tex - 16;
		}
		if (face == Facing::DOWN) {
			return 13 * 16;//tex + 16;
		}
		return super::getTexture(face, data);
	}

	static const int TYPE_DEFAULT     = 0;
	static const int TYPE_HEIROGLYPHS = 1;
	static const int TYPE_SMOOTHSIDE  = 2;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__SandStoneTile_H__*/
