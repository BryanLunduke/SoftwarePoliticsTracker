#ifndef NET_MINECRAFT_WORLD_LEVEL_BIOME__Biome_H__
#define NET_MINECRAFT_WORLD_LEVEL_BIOME__Biome_H__

//package net.minecraft.world.level.biome;

#include <string>
#include <vector>

#include "../../../util/WeighedRandom.h"

class Feature;
class MobCategory;

class Biome
{
public:
    static Biome* rainForest;
    static Biome* swampland;
    static Biome* seasonalForest;
    static Biome* forest;
    static Biome* savanna;
    static Biome* shrubland;
    static Biome* taiga;
    static Biome* desert;
    static Biome* plains;
    static Biome* iceDesert;
    static Biome* tundra;

    class MobSpawnerData: public WeighedRandom::WeighedRandomItem
	{
		typedef WeighedRandom::WeighedRandomItem super;
	public:
        int mobClassId;
        int minCount;
        int maxCount;

		MobSpawnerData()
		{}

        MobSpawnerData(int mobClassId, int probabilityWeight, int minCount, int maxCount)
		:	super(probabilityWeight),
            mobClassId(mobClassId),
            minCount(minCount),
            maxCount(maxCount)
		{}
    };
	typedef std::vector<MobSpawnerData> MobList;

protected:
	Biome();

	Biome* setName(const std::string& name);
	Biome* setLeafColor(int leafColor);
	Biome* setColor(int color);
	Biome* setSnowCovered();
	Biome* clearMobs(bool friendlies = true, bool waterFriendlies = true, bool enemies = true);

	MobList _enemies;
    MobList _friendlies;
    MobList _waterFriendlies;
	static MobList _emptyMobList;
public:
	static int defaultTotalEnemyWeight;
	static int defaultTotalFriendlyWeight;

	virtual ~Biome() {}

    static void recalc();
	static void initBiomes();
	static void teardownBiomes();

	virtual Feature* getTreeFeature(Random* random);
	virtual Feature* getGrassFeature(Random* random);

    static Biome* getBiome(float temperature, float downfall);
    static Biome* _getBiome(float temperature, float downfall);

    virtual float adjustScale(float scale);
    virtual float adjustDepth(float depth);

    virtual int getSkyColor(float temp);

	virtual MobList& getMobs(const MobCategory& category);
	virtual float getCreatureProbability();

	std::string name;
	int color;
	char topMaterial;
	char material;
	int leafColor;
private:
	static Biome* map[64*64];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_BIOME__Biome_H__*/
