#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorageSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorageSource_H__

//package net.minecraft.world.level.storage;

#include "LevelStorageSource.h"
#include "MemoryLevelStorage.h"
class ProgressListener;

class MemoryLevelStorageSource: public LevelStorageSource
{
public:
    MemoryLevelStorageSource() {
    }

    std::string getName() {
        return "Memory Storage";
    }

    LevelStorage* selectLevel(const std::string& levelId, bool createPlayerDir) {
        return new MemoryLevelStorage();
    }

    //List<LevelSummary> getLevelList() {
    //    return /*new*/ ArrayList<LevelSummary>();
    //}

    void clearAll() {
    }

    LevelData* getDataTagFor(const std::string& levelId) {
        return NULL;
    }

    bool isNewLevelIdAcceptable(const std::string& levelId) {
        return true;
    }

    void deleteLevel(const std::string& levelId) {
    }

    void renameLevel(const std::string& levelId, const std::string& newLevelName) {
    }

    bool isConvertible(const std::string& levelId) {
        return false;
    }

    bool requiresConversion(const std::string& levelId) {
        return false;
    }

    bool convertLevel(const std::string& levelId, ProgressListener* progress) {
        return false;
    }
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__MemoryLevelStorageSource_H__*/
