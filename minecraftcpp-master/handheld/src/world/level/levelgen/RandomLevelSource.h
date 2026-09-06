#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__

//package net.minecraft.world.level.levelgen;

class Biome;
class Level;
class LevelChunk;

#if 1 || USE_MAP
	#include <map>
	typedef std::map<int, LevelChunk*> ChunkMap;
#else
	#if defined(__APPLE__)
		#include <ext/hash_map>
		namespace std {
			using namespace __gnu_cxx;
		}
	#else
		#include <hash_map>
	#endif
	typedef std::hash_map<int, LevelChunk*> ChunkMap;
#endif


#include "../chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"

class RandomLevelSource: public ChunkSource
{
    static const float SNOW_CUTOFF;
    static const float SNOW_SCALE;

public:
    static const int CHUNK_HEIGHT = 8;
    static const int CHUNK_WIDTH = 4;

	RandomLevelSource(Level* level, long seed, int version, bool spawnMobs);
	~RandomLevelSource();

	bool hasChunk(int x, int y);
    LevelChunk* create(int x, int z);
	LevelChunk* getChunk(int xOffs, int zOffs);

	void prepareHeights(int xOffs, int zOffs, unsigned char* blocks, /*Biome*/void* biomes, float* temperatures);
    void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks, Biome** biomes);
	void postProcess(ChunkSource* parent, int xt, int zt);

    bool tick();

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);

	bool shouldSave();
    std::string gatherStats();

	//bool save(bool force, ProgressListener progressListener) {
private:
	float* getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize);
	void calcWaterDepths(ChunkSource* parent, int xt, int zt);

public:
	//Biome** biomes;
	LargeCaveFeature caveFeature;
	int waterDepths[16+16][16+16];
private:
	ChunkMap chunkMap;

	Random random;
	PerlinNoise lperlinNoise1;
    PerlinNoise lperlinNoise2;
    PerlinNoise perlinNoise1;
    PerlinNoise perlinNoise2;
    PerlinNoise perlinNoise3;
	PerlinNoise scaleNoise;
    PerlinNoise depthNoise;
    PerlinNoise forestNoise;

	Level* level;
	bool spawnMobs;

	float* buffer;
    float sandBuffer[16 * 16];
    float gravelBuffer[16 * 16];
    float depthBuffer[16 * 16];
	float* pnr;
	float* ar;
	float* br;
	float* sr;
	float* dr;
	float* fi;
	float* fis;
    ///*private*/ float[] temperatures;
};

class PerformanceTestChunkSource : public ChunkSource
{
	Level* level;
public:
	PerformanceTestChunkSource(Level* level)
	:	ChunkSource(),
		level(level)
	{

	}

	virtual bool hasChunk(int x, int y) { return true; };
	virtual LevelChunk* getChunk(int x, int z) { return create(x, z); };

    virtual LevelChunk* create(int x, int z);
	virtual void postProcess(ChunkSource* parent, int x, int z) {};

	virtual bool tick() { return false; };

	virtual bool shouldSave() { return false; };

    /**
     * Returns some stats that are rendered when the user holds F3.
     */
	virtual std::string gatherStats() { return "PerformanceTestChunkSource"; };
};


#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__*/
