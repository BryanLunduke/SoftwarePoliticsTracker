#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION)

#include "LevelData.h"
#include "ExternalFileLevelStorageSource.h"
#include "ExternalFileLevelStorage.h"
#include "FolderMethods.h"
#include "../../../platform/file.h"
#include "../../../util/StringUtils.h"
#include <cstdio>
#include <sys/types.h>

#ifdef __APPLE__
#include "MoveFolder.h"
#endif

static const char ILLEGAL_FILE_CHARACTERS[] = {
	'/', '\n', '\r', '\t', '\0', '\f', '`', '?', '*', '\\', '<', '>', '|', '\"', ':'
};

ExternalFileLevelStorageSource::ExternalFileLevelStorageSource(const std::string& externalPath, const std::string& temporaryFilesPath)
:	_hasTempDirectory(temporaryFilesPath != externalPath)
{
#ifndef STANDALONE_SERVER
	const char* p0 = "/games";
	const char* p1 = "/com.mojang";
	const char* p2 = "/minecraftWorlds";
	const char* tree[] = {
		p0, p1, p2
	};

	int treeLength = sizeof(tree) / sizeof(tree[0]);

	createTree(externalPath.c_str(), tree, treeLength);
	
	if (hasTempDirectory())
		createTree(temporaryFilesPath.c_str(), tree, treeLength);

	basePath = externalPath + p0 + p1 + p2;
	tmpBasePath = temporaryFilesPath + p0 + p1 + p2;
#else
	basePath = externalPath;
	tmpBasePath = temporaryFilesPath;
#endif
}

void ExternalFileLevelStorageSource::addLevelSummaryIfExists(LevelSummaryList& dest, const char* dirName)
{
	std::string directory = basePath;
	directory += "/";
	directory += dirName;

	LevelData levelData;

	if (ExternalFileLevelStorage::readLevelData(directory, levelData))
	{
		LevelSummary summary;
		summary.id = dirName;
		summary.name = levelData.levelName;
		summary.lastPlayed = levelData.getLastPlayed();
		summary.sizeOnDisk = (unsigned int)levelData.getSizeOnDisk();
		summary.gameType = levelData.getGameType();

		dest.push_back(summary);
	}
}

void ExternalFileLevelStorageSource::getLevelList(LevelSummaryList& dest)
{
#ifdef WIN32

	WIN32_FIND_DATAA fileData;
	HANDLE hFind;

	std::string searchString = basePath;
	searchString += "/*";

	hFind = FindFirstFileA(searchString.c_str(), &fileData);
	if (hFind != INVALID_HANDLE_VALUE)  {
		do {
			if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				addLevelSummaryIfExists(dest, fileData.cFileName);

		} while (FindNextFileA(hFind, &fileData));
		FindClose(hFind);
	} 


#else
	DIR *dp;
	struct dirent *dirp;
	if((dp  = opendir(basePath.c_str())) == NULL) {
		LOGI("Error listing base folder %s: %d", basePath.c_str(), _errno());
		return;
	}

	while ((dirp = readdir(dp)) != NULL) {
		if (dirp->d_type == DT_DIR)
		{
			addLevelSummaryIfExists(dest, dirp->d_name);
		}
	}
	closedir(dp);
#endif

}

LevelStorage* ExternalFileLevelStorageSource::selectLevel(const std::string& levelId, bool createPlayerDir) {
	return new ExternalFileLevelStorage(levelId, getFullPath(levelId));
}

void ExternalFileLevelStorageSource::deleteLevel( const std::string& levelId )
{
	std::string path = getFullPath(levelId);
	if (!DeleteDirectory(path)) { // If we couldn't delete whole folder, try to remove chunks..
		remove((path + "/chunks.dat").c_str());
		remove((path + "/player.dat").c_str());
		remove((path + "/level.dat").c_str());
	}
}

static std::string getUniqueLevelName(const LevelSummaryList& levels, const std::string& level )
{
	std::set<std::string> Set;
	for (unsigned int i = 0; i < levels.size(); ++i)
		Set.insert(levels[i].id);

	std::string s = level;
	while ( Set.find(s) != Set.end() )
		s += "-";
	return s;
}

void ExternalFileLevelStorageSource::renameLevel( const std::string& oldLevelId_, const std::string& newLevelName_ )
{
#define _renFULLPATH(s) ((basePath + "/" + s).c_str())
    bool isTempFile = (TempLevelId == oldLevelId_);
    std::string oldFolder = getFullPath(oldLevelId_);

	if (_access(oldFolder.c_str(), 0) != 0) {
		LOGI("Couldn't access %s\n", oldFolder.c_str());
		return;
	}

	std::string levelName = Util::stringTrim(newLevelName_);
	std::string levelId = levelName;
	for (int i = 0; i < sizeof(ILLEGAL_FILE_CHARACTERS) / sizeof(char); ++i)
		levelId = Util::stringReplace(levelId, std::string(1, ILLEGAL_FILE_CHARACTERS[i]), "");

	LevelSummaryList levels;
	getLevelList(levels);
	levelId = getUniqueLevelName(levels, levelId);

	bool couldRename = false;
	if (hasTempDirectory() && isTempFile) {
#ifdef __APPLE__
		std::string newFolder = basePath + "/" + levelId;
		moveFolder(oldFolder, newFolder);
        couldRename = (_access(newFolder.c_str(), 0) == 0);
#endif
	}

    couldRename = couldRename || rename(_renFULLPATH(oldLevelId_), _renFULLPATH(levelId)) == 0;
	if (!couldRename) // != 0: fail
		levelId = oldLevelId_; // Try to rewrite the level name anyway

	// Rename the level name and write back to file
	LevelData levelData;
	ExternalFileLevelStorage::readLevelData(_renFULLPATH(levelId), levelData);
	levelData.setLevelName(newLevelName_);
	ExternalFileLevelStorage::saveLevelData(_renFULLPATH(levelId), levelData, NULL);
}

std::string ExternalFileLevelStorageSource::getName()
{
	return "External File Level Storage";
}

LevelData* ExternalFileLevelStorageSource::getDataTagFor( const std::string& levelId )
{
	return NULL;
}

bool ExternalFileLevelStorageSource::isNewLevelIdAcceptable( const std::string& levelId )
{
	return true;
}

std::string ExternalFileLevelStorageSource::getFullPath(const std::string& levelId) {
    return ((TempLevelId == levelId)? tmpBasePath : basePath) + "/" + levelId;
}


#endif /*DEMO_MODE*/
