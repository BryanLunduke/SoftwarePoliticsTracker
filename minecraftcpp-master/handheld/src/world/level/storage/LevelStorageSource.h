#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorageSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorageSource_H__

//package net.minecraft.world.level.storage;

#include <string>
#include <vector>

class ProgressListener;
class LevelData;
class LevelStorage;

struct LevelSummary
{
	std::string id;
	std::string name;
	int lastPlayed;
	int gameType;
	unsigned int sizeOnDisk;

	bool operator<(const LevelSummary& rhs) const {
		return lastPlayed > rhs.lastPlayed;
	}
};
typedef std::vector<LevelSummary> LevelSummaryList;

class LevelStorageSource
{
public:
	static const std::string TempLevelId;

    virtual ~LevelStorageSource() {}
    
    virtual std::string getName() = 0;
	virtual void getLevelList(LevelSummaryList& dest) {};

    virtual LevelData* getDataTagFor(const std::string& levelId) = 0;
	virtual LevelStorage* selectLevel(const std::string& levelId, bool createPlayerDir) = 0;
    /**
     * Tests if a levelId can be used to store a level. For example, a levelId
     * can't be called COM1 on Windows systems, because that is a reserved file
     * handle.
     * <p>
     * Also, a new levelId may not overwrite an existing one.
     * 
     * @param levelId
     * @return
     */
    virtual bool isNewLevelIdAcceptable(const std::string& levelId) = 0;

	virtual void clearAll() = 0;
    virtual void deleteLevel(const std::string& levelId) = 0;
    virtual void renameLevel(const std::string& levelId, const std::string& newLevelName) = 0;

    virtual bool isConvertible(const std::string& levelId) = 0;
    virtual bool requiresConversion(const std::string& levelId) = 0;
    virtual bool convertLevel(const std::string& levelId, ProgressListener* progress) = 0;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__LevelStorageSource_H__*/
