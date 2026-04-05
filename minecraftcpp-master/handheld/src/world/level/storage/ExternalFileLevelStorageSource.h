#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION)

#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__ExternalFileLevelStorageSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__ExternalFileLevelStorageSource_H__

//package net.minecraft.world.level.storage;

#include "LevelStorageSource.h"
#include "MemoryLevelStorage.h"

class ProgressListener;

class ExternalFileLevelStorageSource: public LevelStorageSource
{
public:
	ExternalFileLevelStorageSource(const std::string& externalPath, const std::string& temporaryFilesPath);

    std::string getName();
	void getLevelList(LevelSummaryList& dest);

    LevelStorage* selectLevel(const std::string& levelId, bool createPlayerDir);
    LevelData* getDataTagFor(const std::string& levelId);

    bool isNewLevelIdAcceptable(const std::string& levelId);

	void clearAll() {}
    void deleteLevel(const std::string& levelId);
    void renameLevel(const std::string& levelId, const std::string& newLevelName);

    bool isConvertible(const std::string& levelId) { return false; }
    bool requiresConversion(const std::string& levelId) { return false; }
    bool convertLevel(const std::string& levelId, ProgressListener* progress) { return false; }
private:
	void addLevelSummaryIfExists(LevelSummaryList& dest, const char* dirName);
	bool hasTempDirectory() { return _hasTempDirectory; }
    std::string getFullPath(const std::string& levelId);

	std::string basePath;
	std::string tmpBasePath;
	bool _hasTempDirectory;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__ExternalFileLevelStorageSource_H__*/

#endif /*DEMO_MODE*/
