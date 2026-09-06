#ifndef NET_MINECRAFT_WORLD_LEVEL__MobSpawner_H__
#define NET_MINECRAFT_WORLD_LEVEL__MobSpawner_H__

//package net.minecraft.world.level;

#include <map>
#include <vector>

class Mob;
class Level;
class Biome;
class Random;
class MobCategory;
class TilePos;
class ChunkPos;

//
//@NOTE: This class is FINAL/sealed! (at least in java)
//
class MobSpawner
{
public:
    static int tick(Level* level, bool spawnEnemies, bool spawnFriendlies);

	static void postProcessSpawnMobs(Level* level, Biome* biome, int xo, int zo, int cellWidth, int cellHeight, Random* random);
    static void finalizeMobSettings(Mob* mob, Level* level, float xx, float yy, float zz);
	
    static bool isSpawnPositionOk(const MobCategory& category, Level* level, int x, int y, int z);
	static TilePos getRandomPosWithin(Level* level, int xo, int zo);

	static bool addMob(Level* level, Mob* mob, float x, float y, float z, float yRot, float xRot, bool force);
	static void makeBabyMob(Mob* mob, float probability);
protected:
    static const std::vector<int> bedEnemies;
private:
    static const int MIN_SPAWN_DISTANCE = 24;

	static std::map<ChunkPos, bool> chunksToPoll;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL__MobSpawner_H__*/
