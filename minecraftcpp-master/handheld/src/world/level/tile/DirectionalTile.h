#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__DirectionalTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__DirectionalTile_H__
#include "Tile.h"
class Material;
class DirectionalTile : public Tile {
public:
	static const int DIRECTION_MASK = 0x3;
	static const int DIRECTION_INV_MASK = 0xc;
	static int getDirection(int data) {
		return data & DIRECTION_MASK;
	}
protected:
	DirectionalTile(int id, int tex, const Material* material) : Tile(id, tex, material) {}
	DirectionalTile(int id, const Material* material) : Tile(id, material) {}
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__DirectionalTile_H__ */