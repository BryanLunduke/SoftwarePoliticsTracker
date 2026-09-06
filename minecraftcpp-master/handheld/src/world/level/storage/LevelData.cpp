#include "LevelData.h"

LevelData::LevelData()
:	xSpawn(128),
	ySpawn(64),
	zSpawn(128),

	seed(0),
	lastPlayed(0),
	generatorVersion(SharedConstants::GeneratorVersion),
	time(0),
	dimension(Dimension::NORMAL),
	playerDataVersion(-1),
	storageVersion(0),
	gameType(GameType::Default),
	loadedPlayerTag(NULL)
{
	//LOGI("ctor 1: %p\n", this);
	spawnMobs = (gameType == GameType::Survival);
}

LevelData::LevelData( const LevelSettings& settings, const std::string& levelName, int generatorVersion /*= -1*/ )
:	seed(settings.getSeed()),
	gameType(settings.getGameType()),
	levelName(levelName),
	xSpawn(128),
	ySpawn(64),
	zSpawn(128),
	lastPlayed(0),
	time(0),
	dimension(Dimension::NORMAL),
	playerDataVersion(-1),
	loadedPlayerTag(NULL)
{
	//LOGI("ctor 2: %p\n", this);

	if (generatorVersion < 0)
		generatorVersion = SharedConstants::GeneratorVersion;

	this->generatorVersion = generatorVersion;
	spawnMobs = (gameType == GameType::Survival);
}

LevelData::LevelData( CompoundTag* tag )
:	loadedPlayerTag(NULL)
{
	//LOGI("ctor 3: %p (%p)\n", this, tag);
	getTagData(tag);
}

LevelData::LevelData( const LevelData& rhs )
:	seed(rhs.seed),
	gameType(rhs.gameType),
	levelName(rhs.levelName),
	xSpawn(rhs.xSpawn),
	ySpawn(rhs.ySpawn),
	zSpawn(rhs.zSpawn),
	lastPlayed(rhs.lastPlayed),
	time(rhs.time),
	dimension(rhs.dimension),
	storageVersion(rhs.storageVersion),
	playerDataVersion(rhs.playerDataVersion),
	generatorVersion(rhs.generatorVersion),
	spawnMobs(rhs.spawnMobs),
	loadedPlayerTag(NULL),
	playerData(rhs.playerData)
{
	//LOGI("c-ctor: %p (%p)\n", this, &rhs);
	setPlayerTag(rhs.loadedPlayerTag);
	//PlayerData playerData;
}

LevelData& LevelData::operator=( const LevelData& rhs )
{
	//LOGI("as-op: %p (%p)\n", this, &rhs);
	if (this != &rhs) {
		seed		= rhs.seed;
		gameType	= rhs.gameType;
		levelName	= rhs.levelName;
		xSpawn		= rhs.xSpawn;
		ySpawn		= rhs.ySpawn;
		zSpawn		= rhs.zSpawn;
		lastPlayed	= rhs.lastPlayed;
		time		= rhs.time;
		dimension	= rhs.dimension;
		spawnMobs	= rhs.spawnMobs;
		playerData  = rhs.playerData;
		playerDataVersion	= rhs.playerDataVersion;
		generatorVersion	= rhs.generatorVersion;
		storageVersion		= rhs.storageVersion;
		setPlayerTag(rhs.loadedPlayerTag);
	}

	return *this;
}

LevelData::~LevelData()
{
	//LOGI("dtor: %p\n", this);
	setPlayerTag(NULL);
}

void LevelData::v1_write( RakNet::BitStream& bitStream )
{
	bitStream.Write(seed);
	bitStream.Write(xSpawn);
	bitStream.Write(ySpawn);
	bitStream.Write(zSpawn);
	bitStream.Write(time);
	bitStream.Write(sizeOnDisk);
	bitStream.Write(getEpochTimeS());
	RakNet::RakString rakName = levelName.c_str();
	bitStream.Write(rakName);
	//LOGI("WBS: %d, %d, %d, %d, %d, %d\n", seed, xSpawn, ySpawn, zSpawn, time, sizeOnDisk);
}

void LevelData::v1_read( RakNet::BitStream& bitStream, int storageVersion )
{
	this->storageVersion = storageVersion;
	bitStream.Read(seed);
	bitStream.Read(xSpawn);
	bitStream.Read(ySpawn);
	bitStream.Read(zSpawn);
	bitStream.Read(time);
	bitStream.Read(sizeOnDisk);
	bitStream.Read(lastPlayed);
	RakNet::RakString rakName;
	bitStream.Read(rakName);
	levelName = rakName.C_String();
	//LOGI("RBS: %d, %d, %d, %d, %d, %d\n", seed, xSpawn, ySpawn, zSpawn, time, sizeOnDisk);
}

// Caller's responsibility to destroy this Tag
CompoundTag* LevelData::createTag()
{
	CompoundTag* tag = new CompoundTag();
	CompoundTag* player = loadedPlayerTag? (CompoundTag*)loadedPlayerTag->copy() : NULL;
	setTagData(tag, player);
	return tag;
}

CompoundTag* LevelData::createTag( const std::vector<Player*>& players )
{
	CompoundTag* tag = new CompoundTag();

	Player* player = NULL;
	CompoundTag* playerTag = NULL;

	if (!players.empty()) player = players[0];
	if (player != NULL) {
		playerTag = new CompoundTag();
		player->saveWithoutId(playerTag);
	}
	setTagData(tag, playerTag);

	return tag;
}

void LevelData::setTagData( CompoundTag* tag, CompoundTag* playerTag )
{
	if (!tag) return;
	tag->putLong("RandomSeed", seed);
	tag->putInt("GameType", gameType);
	tag->putInt("SpawnX", xSpawn);
	tag->putInt("SpawnY", ySpawn);
	tag->putInt("SpawnZ", zSpawn);
	tag->putLong("Time", time);
	tag->putLong("SizeOnDisk", sizeOnDisk);
	tag->putLong("LastPlayed", getEpochTimeS());
	tag->putString("LevelName", levelName);
	tag->putInt("StorageVersion", storageVersion);
	tag->putInt("Platform", 2);

	if (playerTag != NULL) {
		tag->putCompound("Player", playerTag);
	}
}

void LevelData::getTagData( const CompoundTag* tag )
{
	if (!tag) return;
	seed = (long)tag->getLong("RandomSeed");
	gameType = tag->getInt("GameType");
	xSpawn = tag->getInt("SpawnX");
	ySpawn = tag->getInt("SpawnY");
	zSpawn = tag->getInt("SpawnZ");
	time = (long)tag->getLong("Time");
	lastPlayed = (int)tag->getLong("LastPlayed");
	sizeOnDisk = (int)tag->getLong("SizeOnDisk");
	levelName = tag->getString("LevelName");
	storageVersion = tag->getInt("StorageVersion");

	spawnMobs = (gameType == GameType::Survival);

	if (tag->contains("Player", Tag::TAG_Compound)) {
		setPlayerTag(tag->getCompound("Player"));

		//dimension = loadedPlayerTag.getInt("Dimension");
	}
}

void LevelData::setPlayerTag( CompoundTag* tag )
{
	if (loadedPlayerTag) {
		loadedPlayerTag->deleteChildren();
		delete loadedPlayerTag;
		loadedPlayerTag = NULL;
	}
	if (tag)
		loadedPlayerTag = (CompoundTag*)tag->copy();
}

long LevelData::getSeed() const
{
	return seed;
}

int LevelData::getXSpawn() const
{
	return xSpawn;
}

int LevelData::getYSpawn() const
{
	return ySpawn;
}

int LevelData::getZSpawn() const
{
	return zSpawn;
}

long LevelData::getTime() const
{
	return time;
}

long LevelData::getSizeOnDisk() const
{
	return sizeOnDisk;
}

CompoundTag* LevelData::getLoadedPlayerTag()
{
	return loadedPlayerTag;
}

void LevelData::setLoadedPlayerTo( Player* p )
{
	if (playerDataVersion == 1)
		playerData.loadPlayer(p);
}

int LevelData::getDimension()
{
	return dimension;
}

void LevelData::setSeed( long seed )
{
	this->seed = seed;
}

void LevelData::setXSpawn( int xSpawn )
{
	this->xSpawn = xSpawn;
}

void LevelData::setYSpawn( int ySpawn )
{
	this->ySpawn = ySpawn;
}

void LevelData::setZSpawn( int zSpawn )
{
	this->zSpawn = zSpawn;
}

void LevelData::setTime( long time )
{
	this->time = time;
}

void LevelData::setSizeOnDisk( long sizeOnDisk )
{
	this->sizeOnDisk = sizeOnDisk;
}

void LevelData::setLoadedPlayerTag( CompoundTag* playerTag )
{
	LOGI("set-p: %p (%p <- %p)\n", this, loadedPlayerTag, playerTag);
	if (loadedPlayerTag) {
		loadedPlayerTag->deleteChildren();
		delete loadedPlayerTag;
	}
	loadedPlayerTag = playerTag;
}

void LevelData::setDimension( int dimension )
{
	this->dimension = dimension;
}

void LevelData::setSpawn( int xSpawn, int ySpawn, int zSpawn )
{
	this->xSpawn = xSpawn;
	this->ySpawn = ySpawn;
	this->zSpawn = zSpawn;
}

std::string LevelData::getLevelName()
{
	return levelName;
}

void LevelData::setLevelName( const std::string& levelName )
{
	this->levelName = levelName;
}

int LevelData::getGeneratorVersion() const
{
	return generatorVersion;
}

void LevelData::setGeneratorVersion( int version )
{
	this->generatorVersion = version;
}

long LevelData::getLastPlayed() const
{
	return lastPlayed;
}

int LevelData::getStorageVersion() const
{
	return storageVersion;
}

void LevelData::setStorageVersion( int version )
{
	storageVersion = version;
}

int LevelData::getGameType() const
{
	return gameType;
}

void LevelData::setGameType( int type )
{
	gameType = type;
}

bool LevelData::getSpawnMobs() const
{
	return spawnMobs;
}

void LevelData::setSpawnMobs( bool doSpawn )
{
	spawnMobs = doSpawn;
}
