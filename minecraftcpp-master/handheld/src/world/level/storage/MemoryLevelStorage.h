#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorage_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorage_H__

//package net.minecraft.world.level.storage;

#include <vector>

#include "LevelStorage.h"
#include "../chunk/storage/MemoryChunkStorage.h"

class Player;
class Dimension;

class MemoryLevelStorage: public LevelStorage //public PlayerIO
{
public:
    MemoryLevelStorage()
	:	_storage(NULL)
	{}

	~MemoryLevelStorage() {
		delete _storage;
	}

    LevelData* prepareLevel(Level* level) {
        return NULL;
    }

    void checkSession() //throws LevelConflictException
	{}

    ChunkStorage* createChunkStorage(Dimension* dimension) {
		if (_storage) {
			LOGW(">WARNING< Creating a MemoryChunkStorage over another (#%p). A memory leak will occur.\n", _storage);
		}

		return _storage = new MemoryChunkStorage();
    }

    void saveLevelData(LevelData& levelData, std::vector<Player*>* players) {}

    void closeAll() {}

    void save(Player* player) {}
    void load(Player* player) {}

    /* CompoundTag loadPlayerDataTag(std::string playerName) {
        return NULL;
    } */
	MemoryChunkStorage* _storage;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorage_H__*/
