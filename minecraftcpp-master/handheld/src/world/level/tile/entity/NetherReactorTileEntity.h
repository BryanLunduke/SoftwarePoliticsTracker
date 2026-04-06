#ifndef NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__NetherReactorTileEntity_H__
#define NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__NetherReactorTileEntity_H__
#include "../../../Pos.h"
#include "TileEntity.h"
class NetherReactorTileEntity : public  TileEntity {
	typedef TileEntity super;
	static const int NUM_PIG_ZOMBIE_SLOTS = 3;
public:
	NetherReactorTileEntity();
	bool shouldSave();
	void lightItUp(int x, int y, int z);

	void buildDome(int x, int y, int z);
	void clearDomeSpace( int x, int y, int z );

	void tick();

	void finishReactorRun();

	bool save(CompoundTag* tag);
	void load(CompoundTag* tag);
	int getNumEnemiesPerLevel(int curLevel);
	int getNumItemsPerLevel(int curLevel);
	void spawnItems(int numItems);
	std::string getName() const;
	void spawnEnemy();
	void spawnItem();
	ItemInstance getSpawnItem();

	ItemInstance GetLowOddsSpawnItem();
	bool checkLevelChange( int progress );
	int numOfFreeEnemySlots();
	void trySpawnPigZombies( int maxNumOfEnemies, int maxToSpawn );
	void tickGlowingRedstoneTransformation( int currentTime );
	void turnLayerToGlowingObsidian( int layer, const int type );
	void turnGlowingObsidianLayerToObsidian( int layer );
	Vec3 getSpawnPosition( float minDistance, float varibleDistance, float offset );
	void buildHollowedVolume( int x, int y, int z, int expandWidth, int height, const int wallTileId, const int clearTileId );
	void buildFloorVolume( int x, int y, int z, int expandWidth, int heightm, const int tileId );
	void buildCrockedRoofVolume( bool inverted, int x, int y, int z, int expandWidth, int height, const int tileId );

	bool isEdge( int curX, int expandWidth, int curZ );
	void deterioateDome( int x, int y, int z );
	void deterioateCrockedRoofVolume( bool inverted, int x, int y, int z, int expandWidth, int height, int tileId );
	void deterioateHollowedVolume( int x, int y, int z, int expandWidth, int height, int tileId );
	bool playersAreCloseBy();
	void killPigZombies();
private:
	bool isInitialized;
	bool hasFinished;
	int curLevel;
	short progress;
};

#endif /* NET_MINECRAFT_WORLD_LEVEL_TILE_ENTITY__NetherReactorTileEntity_H__ */
