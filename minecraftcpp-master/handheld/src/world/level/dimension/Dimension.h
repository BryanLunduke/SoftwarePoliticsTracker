#ifndef NET_MINECRAFT_WORLD_LEVEL_DIMENSION__Dimension_H__
#define NET_MINECRAFT_WORLD_LEVEL_DIMENSION__Dimension_H__

//package net.minecraft.world.level.dimension;

#include "../../phys/Vec3.h"

class Level;
class BiomeSource;
class ChunkSource;

class Dimension
{
public:
	static const int NORMAL = 0;
	static const int NORMAL_DAYCYCLE = 10;

	Dimension();
	virtual ~Dimension();
    virtual void init(Level* level);

	//@fix @port The caller is responsible for this ChunkSource, I presume
    virtual ChunkSource* createRandomLevelSource();

    virtual bool isValidSpawn(int x, int z);
	virtual bool isNaturalDimension() {
		return false;
	}

    virtual float getTimeOfDay(long time, float a);
    virtual float* getSunriseColor(float td, float a);
    virtual Vec3 getFogColor(float td, float a);

    virtual bool mayRespawn();

	// @fix @port Caller is responsible (+ move this to a "factory method" outside?)
	// @NOTE: RIGHT NOW, Level deletes the dimension.
    static Dimension* getNew(int id);

protected:
	virtual void updateLightRamp();
	virtual void init();

public:
	Level* level;
	BiomeSource* biomeSource;
	bool foggy;
	bool ultraWarm;
	bool hasCeiling;
	float brightnessRamp[16];//Level::MAX_BRIGHTNESS + 1];
	int id;
protected:
	static const long fogColor = 0x80daff;//0x406fe5;//0xc0d8ff;
	float sunriseCol[4];
};

class LevelData;
class DimensionFactory
{
public:
	static Dimension* createDefaultDimension(LevelData* data);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_DIMENSION__Dimension_H__*/
