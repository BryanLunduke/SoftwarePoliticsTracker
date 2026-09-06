#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__BedTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__BedTile_H__

#include "DirectionalTile.h"
#include "../material/Material.h"

class Pos;
class BedTile : public DirectionalTile
{
	typedef DirectionalTile super;
public:
	static const int HEAD_PIECE_DATA = 0x8;
	static const int OCCUPIED_DATA = 0x4;
	static const int HEAD_DIRECTION_OFFSETS[4][2];

	BedTile(int id);

	bool use(Level* level, int x, int y, int z, Player* player);

	int getTexture(int face, int data);
	int getRenderShape();
	int getRenderLayer();
	bool isCubeShaped();
	bool isSolidRender();

	void updateShape(LevelSource* level, int x, int y, int z);
	void setShape();

	void neighborChanged(Level* level, int x, int y, int z, int type);

	int getResource(int data, Random* random);
	void spawnResources(Level* level, int x, int y, int z, int data, float odds);

	static bool isHeadPiece( int data );
	static bool isOccupied(int data);
	static void setOccupied( Level* level, int x, int y, int z, bool occupied );
	static bool findStandUpPosition( Level* level, int x, int y, int z, int skipCount, Pos& position);
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__BedTile_H__ */
