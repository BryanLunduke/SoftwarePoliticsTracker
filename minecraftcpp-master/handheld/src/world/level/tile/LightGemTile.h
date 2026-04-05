#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__LightGemTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__LightGemTile_H__
#include "Tile.h"
class LightGemTile : public Tile {
	typedef Tile super;
public:
	LightGemTile(int id, int tex, const Material* material);
	int getResourceCount(Random* random);
	int getResource(int data, Random* random);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__LightGemTile_H__ */