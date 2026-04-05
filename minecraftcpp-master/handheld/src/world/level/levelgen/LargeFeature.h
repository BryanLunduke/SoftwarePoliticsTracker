#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__

//package net.minecraft.world.level.levelgen;

#include "../../../util/Random.h"

class Random;
class Level;
class ChunkSource;

class LargeFeature
{
public:
	LargeFeature();
	virtual ~LargeFeature();

    virtual void apply(ChunkSource* chunkSource, Level* level, int xOffs, int zOffs, unsigned char* blocks, int blocksSize);

protected:
    virtual void addFeature(Level* level, int x, int z, int xOffs, int zOffs, unsigned char* blocks, int blocksSize) = 0;

	int radius;
    Random random;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__*/
