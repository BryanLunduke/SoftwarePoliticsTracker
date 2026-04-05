#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorage_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorage_H__

//package net.minecraft.world.level.storage;

#include <vector>
#include <string>

class LevelData;
class ChunkStorage;
class Dimension;
class Player;
class Level;
class LevelChunk;

class LevelStorage
{
public:
	virtual ~LevelStorage() {}
	
    virtual LevelData* prepareLevel(Level* level) = 0;

	virtual ChunkStorage* createChunkStorage(Dimension* dimension) = 0;

    virtual void saveLevelData(LevelData& levelData, std::vector<Player*>* players) = 0;
	virtual void saveLevelData(LevelData& levelData) {
		saveLevelData(levelData, NULL);
	}

    virtual void closeAll() = 0;

	virtual void saveGame(Level* level) {}
	virtual void loadEntities(Level* level, LevelChunk* levelChunk) {}

	//void checkSession() throws LevelConflictException;
	//PlayerIO getPlayerIO();
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorage_H__*/
