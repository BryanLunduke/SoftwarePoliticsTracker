#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__DoorTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__DoorTile_H__

//package net.minecraft.world.level->tile;

#include "Tile.h"
#include "../material/Material.h"
#include "../../../util/Random.h"

class Item;
class Level;
class LevelSource;
class Player;


class DoorTile: public Tile
{
	typedef Tile super;
public:
	static const int UPPER_BIT = 8;
	static const int C_DIR_MASK = 3;
	static const int C_OPEN_MASK = 4;
	static const int C_LOWER_DATA_MASK = 7;
	static const int C_IS_UPPER_MASK = 8;
	static const int C_RIGHT_HINGE_MASK = 16;

	DoorTile(int id, const Material* material);
	int getTexture(LevelSource* level, int x, int y, int z, int face);

	bool blocksLight();
    bool isSolidRender();
    bool isCubeShaped();
    int getRenderShape();
	int getRenderLayer();

	AABB getTileAABB(Level* level, int x, int y, int z);
    AABB* getAABB(Level* level, int x, int y, int z);

    void updateShape(LevelSource* level, int x, int y, int z);
    void setShape(int compositeData);

    void attack(Level* level, int x, int y, int z, Player* player);
    bool use(Level* level, int x, int y, int z, Player* player);

    void setOpen(Level* level, int x, int y, int z, bool shouldOpen);
	static bool isOpen(LevelSource* level, int x, int y, int z);

	void neighborChanged(Level* level, int x, int y, int z, int type);

    int getResource(int data, Random* random);

    HitResult clip(Level* level, int xt, int yt, int zt, const Vec3& a, const Vec3& b);

    int getDir(LevelSource* level, int x, int y, int z);

    bool mayPlace(Level* level, int x, int y, int z, unsigned char face);

	static int getCompositeData(LevelSource* level, int x, int y, int z);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_TILE__DoorTile_H__*/
