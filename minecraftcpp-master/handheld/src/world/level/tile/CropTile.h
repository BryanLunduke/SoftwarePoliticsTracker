#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE__CropTile_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE__CropTile_H__

#include "Bush.h"

class CropTile : public Bush
{
	typedef Bush super;
public:
	CropTile(int id, int tex);
	bool mayPlaceOn(int tile);
	void tick(Level* level, int x, int y, int z, Random* random);

	int getTexture(LevelSource* level, int x, int y, int z, int face);
	int getTexture( int face, int data );

	int getRenderShape();

	void spawnResources(Level* level, int x, int y, int z, int data, float odds);
	int getResource(int data, Random* random);
	int getResourceCount(Random* random);

	void growCropsToMax(Level* level, int x, int y, int z);
private:
	float getGrowthSpeed(Level* level, int x, int y, int z);
	
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE__CropTile_H__ */