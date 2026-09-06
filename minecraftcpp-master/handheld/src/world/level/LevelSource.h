#ifndef NET_MINECRAFT_WORLD_LEVEL__LevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL__LevelSource_H__

//package net.minecraft.world.level;

/*
class BiomeSource;
class TileEntity;
*/
class Material;
class Biome;

class LevelSource
{
public:
	virtual ~LevelSource() {}

	virtual int getTile(int x, int y, int z) = 0;
	virtual bool isEmptyTile(int x, int y, int z) = 0;

    //virtual TileEntity* getTileEntity(int x, int y, int z) = 0;

    virtual float getBrightness(int x, int y, int z) = 0;

    virtual int getData(int x, int y, int z) = 0;

    virtual const Material* getMaterial(int xx, int i, int zz) = 0;

    virtual bool isSolidRenderTile(int x, int i, int z) = 0;
	virtual bool isSolidBlockingTile(int x, int i, int z) = 0;

    //virtual BiomeSource* getBiomeSource() = 0;
	virtual Biome* getBiome(int x, int z) = 0;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__LevelSource_H__*/
