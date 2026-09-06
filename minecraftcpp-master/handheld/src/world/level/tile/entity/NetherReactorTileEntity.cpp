#include "NetherReactorTileEntity.h"
#include "../../../../nbt/CompoundTag.h"
#include "../../../../SharedConstants.h"
#include "../../../phys/Vec3.h"
#include "../../../level/Level.h"
#include "../../../level/MobSpawner.h"
#include "../../../entity/MobFactory.h"
#include "../NetherReactor.h"
#include "../../../entity/Entity.h"
#include "../../../Difficulty.h"
#include "../../../phys/AABB.h"
#include "../NetherReactorPattern.h"
NetherReactorTileEntity::NetherReactorTileEntity()
	: super(TileEntityType::NetherReactor)
	, isInitialized(false)
	, progress(0)
	, curLevel(0)
	, hasFinished(false)
{ }

bool NetherReactorTileEntity::shouldSave() {
	return true;
}

void NetherReactorTileEntity::lightItUp( int x, int y, int z ) {
	//if(!isInitilized && !hasFinished) {
	curLevel = 0;
	NetherReactor::setPhase(level, x, y, z, 1);
		isInitialized = true;
		//clearDomeSpace(x, y, z);
		buildDome(x, y, z);
		
		level->setNightMode(true);
	//}
}

void NetherReactorTileEntity::tick() {
	if(level->isClientSide)
		return;
	if(progress < 0) {
		remove = true;
	}
	if(isInitialized && !hasFinished) {
		progress++;
		if(progress % SharedConstants::TicksPerSecond == 0) {
			int currentTime = progress / SharedConstants::TicksPerSecond;
			if(currentTime < 10) {
				tickGlowingRedstoneTransformation(currentTime);
			}
			if(currentTime > 42 && currentTime <= 45) {
				// start with the top layer and move down
				int currentLayer = 45 - currentTime;
				turnGlowingObsidianLayerToObsidian(currentLayer);
			}
			if(checkLevelChange(progress / SharedConstants::TicksPerSecond)) {
				curLevel++;
				spawnItems(getNumItemsPerLevel(curLevel));
				trySpawnPigZombies(NUM_PIG_ZOMBIE_SLOTS, getNumEnemiesPerLevel(curLevel));
			}
		}	
		if(progress > SharedConstants::TicksPerSecond * 46) {
			finishReactorRun();
		}
	} else if(hasFinished) {
		if(progress % SharedConstants::TicksPerSecond * 60 == 0) {
			if(playersAreCloseBy()) {
				trySpawnPigZombies(2, 3);
			} else {
				killPigZombies();
			}
		}
	}
}

void NetherReactorTileEntity::load( CompoundTag* tag ) {
	super::load(tag);
	isInitialized = tag->getBoolean("IsInitialized");
	if(isInitialized) {
		progress = tag->getShort("Progress");
		hasFinished = tag->getBoolean("HasFinished");
	}
}

bool NetherReactorTileEntity::save( CompoundTag* tag ) {
	super::save(tag);
	tag->putBoolean("IsInitialized", isInitialized);
	tag->putShort("Progress", progress);
	tag->putBoolean("HasFinished", hasFinished);
	if(isInitialized && !hasFinished)
		level->setNightMode(true);
	return true;
}

std::string NetherReactorTileEntity::getName() const {
	return "NetherReactor";
}

int NetherReactorTileEntity::getNumEnemiesPerLevel( int curLevel ) {
	if(curLevel == 0)
		return 3;
	else if(curLevel < 4)
		return 2;
	else if(curLevel < 6)
		return Mth::Max(0, level->random.nextInt(2));
	else
		return Mth::Max(0, level->random.nextInt(1));
}

int NetherReactorTileEntity::getNumItemsPerLevel( int curLevel ) {
	if(curLevel == 0)
		return 3 * 3;
	else if(curLevel < 4)
		return 5 * 3;
	else if(curLevel < 8)
		return Mth::Max(0, level->random.nextInt(14 * 3) - 4);
	else
		return Mth::Max(0, level->random.nextInt(9 * 3) - 2);
}

void NetherReactorTileEntity::spawnItems( int numItems ) {
	for (int ii = 0; ii < numItems ; ii++) {
		spawnItem();
	}
}
Vec3 NetherReactorTileEntity::getSpawnPosition( float minDistance, float varibleDistance, float offset ) {
	float distance = minDistance + level->random.nextFloat() * varibleDistance;
	float rad = level->random.nextFloat() * Mth::TWO_PI;
	return Vec3(sin(rad) * distance + x, offset + y, cos(rad) * distance + z);
}
void NetherReactorTileEntity::spawnEnemy() {
	Mob* mob = MobFactory::CreateMob(MobTypes::PigZombie, level);
	Vec3 enemyPosition = getSpawnPosition(3, 4, -1);
	while(enemyPosition.x < 0 || enemyPosition.z < 0 || enemyPosition.x >= LEVEL_WIDTH || enemyPosition.z >= LEVEL_DEPTH) {
		enemyPosition = getSpawnPosition(3, 4, -1);
	}
	MobSpawner::addMob(level, mob, enemyPosition.x, enemyPosition.y, enemyPosition.z, 0, 0, true);
}



void NetherReactorTileEntity::spawnItem() {
	Vec3 itemPosition= getSpawnPosition(3, 4, -1);
	while(itemPosition.x < 0 || itemPosition.z < 0 || itemPosition.x >= LEVEL_WIDTH || itemPosition.z >= LEVEL_DEPTH) {
		itemPosition = getSpawnPosition(3, 4, -1);
	}
	ItemEntity* item = new ItemEntity(level, itemPosition.x, itemPosition.y, itemPosition.z, getSpawnItem());

	item->throwTime = 10;
	item->age = item->getLifeTime() - SharedConstants::TicksPerSecond * 30;
	level->addEntity(item);
}

ItemInstance NetherReactorTileEntity::getSpawnItem() {
	int itemType = level->random.nextInt(8);
	switch(itemType) {
	    case 0: return ItemInstance(Item::yellowDust, 3);
		case 1: return ItemInstance(Item::seeds_melon);
		case 2: return ItemInstance(Tile::mushroom1);
		case 3: return ItemInstance(Tile::mushroom2);
		case 4: return ItemInstance(Item::reeds);
		case 5: return ItemInstance(Tile::cactus);
		case 6: return ItemInstance(Item::netherQuartz, 4);
		default: return GetLowOddsSpawnItem();
	}
}

ItemInstance NetherReactorTileEntity::GetLowOddsSpawnItem() {
	if(level->random.nextInt(10) <= 9 ) {
		static Item* items[] = {
			Item::arrow,
			Item::bed,
			Item::bone,
			Item::book,
			Item::bow,
			Item::bowl,
			Item::feather,
			Item::painting,
			Item::door_wood
		};
		int itemIndex = level->random.nextInt(sizeof(items) / 4);
		Item* itemToSpawn = items[itemIndex];
		return ItemInstance(itemToSpawn);
	} else {
		static Tile* tiles[] = {
			Tile::bookshelf
		};
		int tileIndex = level->random.nextInt(sizeof(tiles) / 4);
		Tile* tileToSpawn = tiles[tileIndex];
		return ItemInstance(tileToSpawn);
	}
}

bool NetherReactorTileEntity::checkLevelChange( int progress ) {
	static const int levelChangeTime[] = {10, 13, 20, 22, 25, 30, 34, 36, 38, 40};
	const int count = sizeof(levelChangeTime) / 4;
	for(int a = 0; a < count; ++a) {
		if(levelChangeTime[a] == progress)
			return true;
	}
	return false;
}

void NetherReactorTileEntity::clearDomeSpace( int x, int y, int z ) {
	for(int curX = -12; curX <= 12; ++curX) {
		for(int curY = -3; curY < 40; ++curY) {
			for(int curZ = -12; curZ <= 12; ++curZ) {
				if(curY > 2 || curX < -1 || curX > 1 || curZ < -1 || curZ > 1)
					level->setTile(curX + x, curY + y, curZ + z, 0);
			}
		}
	}
}

void NetherReactorTileEntity::finishReactorRun() {
	NetherReactor::setPhase(level, x, y, z, 2);
	level->setNightMode(false);
	hasFinished = true;
	deterioateDome(x, y, z);
	for(int curX = x - 1; curX <= x + 1; ++curX) {
		for(int curY = y - 1; curY <= y + 1; ++curY) {
			for(int curZ = z - 1; curZ <= z + 1; ++curZ) {
				if(curX != x || curY != y || curZ != z)
					level->setTile(curX, curY, curZ, Tile::obsidian->id);
			}
		}
	}
}

int NetherReactorTileEntity::numOfFreeEnemySlots() {
	int numPigZombiesFound = 0;
	AABB bb((float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f);
	EntityList nearby =  level->getEntities(NULL, bb.grow(7, 7, 7));
	for(EntityList::iterator it = nearby.begin(); it != nearby.end(); ++it) {
		if((*it)->isEntityType(MobTypes::PigZombie) && (*it)->isAlive()) {
			numPigZombiesFound++;
		}
	}
	return NUM_PIG_ZOMBIE_SLOTS - numPigZombiesFound; 
}

void NetherReactorTileEntity::trySpawnPigZombies( int maxNumOfEnemies, int maxToSpawn ) {
	if(level->difficulty == Difficulty::PEACEFUL)
		return;
	int currentNumOfPigZombies = NUM_PIG_ZOMBIE_SLOTS - numOfFreeEnemySlots();
	if(currentNumOfPigZombies < maxNumOfEnemies) {
		for(int a = 0;  a < maxToSpawn && currentNumOfPigZombies < maxNumOfEnemies; ++a) {
			spawnEnemy();
			currentNumOfPigZombies++;
		}
	}
}

void NetherReactorTileEntity::tickGlowingRedstoneTransformation( int currentTime ) {
	switch(currentTime) {
		case 2: return turnLayerToGlowingObsidian(0, Tile::stoneBrick->id);
		case 3: return turnLayerToGlowingObsidian(1, Tile::stoneBrick->id);
		case 4: return turnLayerToGlowingObsidian(2, Tile::stoneBrick->id);
		case 7: return turnLayerToGlowingObsidian(0, Tile::goldBlock->id);
		case 8: return turnLayerToGlowingObsidian(1, Tile::goldBlock->id);
		case 9: return turnLayerToGlowingObsidian(2, Tile::goldBlock->id);
	}
}

void NetherReactorTileEntity::turnLayerToGlowingObsidian( int layer, const int type ) {
	NetherReactorPattern pattern;
	for(int checkX = -1; checkX <= 1; ++checkX) {
		for(int checkZ = -1; checkZ <= 1; ++checkZ) {
			if(pattern.getTileAt(layer, checkX + 1, checkZ + 1) == type) {
				level->setTile(x + checkX, y - 1 + layer, z + checkZ, Tile::glowingObsidian->id);
			}
		}
	}
}

void NetherReactorTileEntity::turnGlowingObsidianLayerToObsidian( int layer ) {
	NetherReactorPattern pattern;
	for(int checkX = -1; checkX <= 1; ++checkX) {
		for(int checkZ = -1; checkZ <= 1; ++checkZ) {
			if(level->getTile(x + checkX, y - 1 + layer, z + checkZ) != Tile::netherReactor->id) {
				level->setTile(x + checkX, y - 1 + layer, z + checkZ, Tile::obsidian->id);
			}
		}
	}
}

void NetherReactorTileEntity::buildDome( int x, int y, int z ) {
	buildFloorVolume(x, y - 3, z, 8, 2, Tile::netherrack->id);
	buildHollowedVolume(x, y - 1, z, 8, 4, Tile::netherrack->id, 0);
	buildFloorVolume(x, y - 1 + 4, z, 8, 1, Tile::netherrack->id);
	buildCrockedRoofVolume(false, x, y - 1 + 5, z, 8, 1, Tile::netherrack->id);
	buildCrockedRoofVolume(true, x, y - 1 + 6, z, 5, 8, Tile::netherrack->id);
	buildCrockedRoofVolume(false, x, y + -1 + 12, z, 3, 14, Tile::netherrack->id);
}

void NetherReactorTileEntity::buildHollowedVolume( int x, int y, int z, int expandWidth, int height, const int wallTileId, const int clearTileId ) {
	for(int curY = 0; curY < height; ++curY) {
		for(int curX = -expandWidth; curX <= expandWidth; ++curX) {
			for(int curZ = -expandWidth; curZ <= expandWidth; ++curZ) {
				if((curX == -expandWidth || curX == expandWidth)
				|| (curZ == -expandWidth || curZ == expandWidth)) {
					level->setTile(curX + x, curY + y, curZ + z, wallTileId);
				} else if(curY > 2 || curX < -1 || curX > 1 || curZ < -1 || curZ > 1) {
					level->setTile(curX + x, curY + y, curZ + z, clearTileId);
				}
			}
		}
	}
}

void NetherReactorTileEntity::buildFloorVolume( int x, int y, int z, int expandWidth, int height, const int tileId ) {
	for(int curY = 0; curY < height; ++curY) {
		for(int curX = -expandWidth; curX <= expandWidth; ++curX) {
			for(int curZ = -expandWidth; curZ <= expandWidth; ++curZ) {
				level->setTile(curX + x, curY + y, curZ + z, tileId);
			}
		}
	}
}

void NetherReactorTileEntity::buildCrockedRoofVolume( bool inverted, int x, int y, int z, int expandWidth, int height, const int tileId ) {
	int fullHeight = height + expandWidth;
	for(int curX = -expandWidth; curX <= expandWidth; ++curX) {
		for(int curZ = -expandWidth; curZ <= expandWidth; ++curZ) {
			int offset = inverted ? ((-curX - curZ) / 2) : ((curX + curZ) / 2);
			int acceptHeight = fullHeight + offset;
			for(int curY = 0; curY < fullHeight + expandWidth; ++curY) {
				if(acceptHeight >= curY
					&& (isEdge(curX, expandWidth, curZ)
					|| acceptHeight == curY )) {
						level->setTile(curX + x, curY + y, curZ + z, tileId);
				}
			}
		}
	}
}

bool NetherReactorTileEntity::isEdge( int curX, int expandWidth, int curZ ) {
	return (curX == -expandWidth || curX == expandWidth)
		|| (curZ == -expandWidth || curZ == expandWidth);
}

void NetherReactorTileEntity::deterioateDome( int x, int y, int z ) {
	deterioateHollowedVolume(x, y - 1, z, 8, 5, 0);
	deterioateCrockedRoofVolume(false, x, y - 1 + 5, z, 8, 1, 0);
	deterioateCrockedRoofVolume(true, x, y - 1 + 6, z, 5, 8, 0);
	deterioateCrockedRoofVolume(false, x, y + -1 + 12, z, 3, 14, 0);
}

void NetherReactorTileEntity::deterioateCrockedRoofVolume( bool inverted, int x, int y, int z, int expandWidth, int height, int tileId ) {
	int fullHeight = height + expandWidth;
	for(int curX = -expandWidth; curX <= expandWidth; ++curX) {
		for(int curZ = -expandWidth; curZ <= expandWidth; ++curZ) {
			int offset = inverted ? ((-curX - curZ) / 2) : ((curX + curZ) / 2);
			int acceptHeight = fullHeight + offset;
			for(int curY = 0; curY < fullHeight + expandWidth; ++curY) {
				if(acceptHeight >= curY
					&& (isEdge(curX, expandWidth, curZ))) {
						if(level->random.nextInt(4) == 0) {
							level->setTile(curX + x, curY + y, curZ + z, tileId);
						}
				}
			}
		}
	}
}

void NetherReactorTileEntity::deterioateHollowedVolume( int x, int y, int z, int expandWidth, int height, int tileId ) {
	for(int curY = 0; curY < height; ++curY) {
		for(int curX = -expandWidth; curX <= expandWidth; ++curX) {
			for(int curZ = -expandWidth; curZ <= expandWidth; ++curZ) {
				if((curX == -expandWidth || curX == expandWidth)
					|| (curZ == -expandWidth || curZ == expandWidth)) {
						if(level->random.nextInt(3) == 0)
							level->setTile(curX + x, curY + y, curZ + z, tileId);
				}
			}
		}
	}
}

bool NetherReactorTileEntity::playersAreCloseBy() {
	int numPlayers = 0;
	AABB bb((float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f);
	EntityList nearby =  level->getEntities(NULL, bb.grow(40, 40, 40));
	for(EntityList::iterator it = nearby.begin(); it != nearby.end(); ++it) {
		if((*it)->isPlayer() && (*it)->isAlive() ) {
			if((*it)->distanceTo((float)x, (float)y, (float)z) < 40)
				numPlayers++;
		}
	}
	return numPlayers != 0;
}

void NetherReactorTileEntity::killPigZombies() {
	AABB bb((float)x, (float)y, (float)z, x + 1.0f, y + 1.0f, z + 1.0f);
	EntityList nearby =  level->getEntities(NULL, bb.grow(40, 40, 40));
	for(EntityList::iterator it = nearby.begin(); it != nearby.end(); ++it) {
		if((*it)->isEntityType(MobTypes::PigZombie)) {
			(*it)->remove();
		}
	}
}
