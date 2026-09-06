#include "LargeFeature.h"
#include "../Level.h"

LargeFeature::LargeFeature() :	radius(8)
{
}

LargeFeature::~LargeFeature()
{
}

void LargeFeature::apply( ChunkSource* chunkSource, Level* level, int xOffs, int zOffs, unsigned char* blocks, int blocksSize )
{
	int r = radius;

	random.setSeed(level->getSeed());
	long xScale = random.nextLong() / 2 * 2 + 1;
	long zScale = random.nextLong() / 2 * 2 + 1;

	for (int x = xOffs - r; x <= xOffs + r; x++) {
		for (int z = zOffs - r; z <= zOffs + r; z++) {
			random.setSeed((x * xScale + z * zScale) ^ level->getSeed());
			addFeature(level, x, z, xOffs, zOffs, blocks, blocksSize);
		}
	}
}

