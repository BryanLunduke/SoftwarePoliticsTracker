#include "MobSpawner.h"

#include <algorithm>

#include "Level.h"
#include "biome/Biome.h"
#include "material/Material.h"
#include "../entity/EntityTypes.h"
#include "../entity/MobFactory.h"
#include "../entity/MobCategory.h"
#include "../entity/player/Player.h"

//#include "../entity/animal/Sheep.h"
//#include "tile/BedTile.h"

std::map<ChunkPos, bool> MobSpawner::chunksToPoll;

static int _bedEnemies[] = {
	MobTypes::Spider,
	MobTypes::Zombie,
	MobTypes::Skeleton,
	MobTypes::PigZombie
};

static const std::vector<int> bedEnemies(_bedEnemies, _bedEnemies + sizeof(_bedEnemies) / sizeof(_bedEnemies[0]));

/*static*/
int MobSpawner::tick(Level* level, bool spawnEnemies, bool spawnFriendlies) {

	//return 0;

	if (!spawnEnemies && !spawnFriendlies) {
        return 0;
    }

    chunksToPoll.clear();
	// Add all chunks as a quick-and-dirty test
	// (The code above is the same as in the java version)
	// This code goes over the whole map one "row" at a time

	// Spawn friendlies == loop over whole map, and disable Monster spawning this tick
	if (spawnFriendlies) {
		spawnEnemies = false;
		for (int i = 0; i < 256; ++i)
			chunksToPoll.insert( std::make_pair( ChunkPos(i>>4, i&15), false) );

	} else {
		// Only spawn mobs, check around one player per tick (@todo: optimize the "count instances of"?)
		static unsigned int _pid = 0;
		if (++_pid >= level->players.size()) _pid = 0;
		if (level->players.size()) {
			Player* p = level->players[_pid];
			int xx = Mth::floor(p->x / 16);
			int zz = Mth::floor(p->z / 16);
			int r = 128 / 16;
			for (int x = -r; x <= r; x++)
			for (int z = -r; z <= r; z++) {
				const int cx = xx + x;
				const int cz = zz + z;
				if (cx >= 0 && cx < 16 && cz >= 0 && cz < 16)
					chunksToPoll.insert(std::make_pair(ChunkPos(cx, cz), false ));
			}
		}
	}

    int count = 0;
    Pos spawnPos = level->getSharedSpawnPos();

	for (int i = 0; i < MobCategory::numValues; ++i) {
		const MobCategory& mobCategory = *MobCategory::values[i];

        if ((mobCategory.isFriendly() && !spawnFriendlies) || (!mobCategory.isFriendly() && !spawnEnemies))
            continue;

		int numMobs = level->countInstanceOfBaseType(mobCategory.getBaseClassId());
		if (numMobs > mobCategory.getMaxInstancesPerLevel())
		    continue;
		//LOGI("NumMobs: %d of Category: %d\n", numMobs, mobCategory.getBaseClassId());
chunkLoop:
		for(std::map<ChunkPos, bool>::iterator it = chunksToPoll.begin(); it != chunksToPoll.end(); ++it) {
			const ChunkPos& cp = it->first;
            TilePos start = getRandomPosWithin(level, cp.x * 16, cp.z * 16);
            int xStart = start.x;
            int yStart = start.y;
            int zStart = start.z;

			if (level->isSolidBlockingTile(xStart, yStart, zStart)) continue;

            if (level->getMaterial(xStart, yStart, zStart) != mobCategory.getSpawnPositionMaterial()) continue;

            int clusterSize = 0;

            for (int dd = 0; dd < 3; dd++) {
                int x = xStart;
                int y = yStart;
                int z = zStart;
                int ss = 6;

                Biome::MobSpawnerData currentMobType;
				int maxCreatureCount = 999;
				int currentCreatureCount = 0;

                for (int ll = 0; ll < 4; ll++) {
					if (currentCreatureCount > maxCreatureCount)
						break;

					x += level->random.nextInt(ss) - level->random.nextInt(ss);
                    y += level->random.nextInt(1) - level->random.nextInt(1);
                    z += level->random.nextInt(ss) - level->random.nextInt(ss);
                    // int y = heightMap[x + z * w] + 1;

                    if (isSpawnPositionOk(mobCategory, level, x, y, z)) {
                        float xx = (float)x + 0.5f;
                        float yy = (float)y;
                        float zz = (float)z + 0.5f;
                        if (level->getNearestPlayer(xx, yy, zz, (float)MIN_SPAWN_DISTANCE) != NULL) {
                            continue;
                        } else {
                            float xd = xx - spawnPos.x;
                            float yd = yy - spawnPos.y;
                            float zd = zz - spawnPos.z;
                            float sd = xd * xd + yd * yd + zd * zd;
                            if (sd < MIN_SPAWN_DISTANCE * MIN_SPAWN_DISTANCE) {
                                continue;
                            }
                        }

						static Stopwatch sw;
						sw.start();

						if (!currentMobType.isValid()) {
                            currentMobType = level->getRandomMobSpawnAt(mobCategory, x, y, z);
							if (!currentMobType.isValid())
                                break;

							// Don't allow monster to spawn (much) more than their defined
							// probability weight.
							if (&mobCategory == &MobCategory::monster) {
								int typeCount = level->countInstanceOfType(currentMobType.mobClassId);
								int typeMax = (int)(1.5f * currentMobType.randomWeight * mobCategory.getMaxInstancesPerLevel()) / Biome::defaultTotalEnemyWeight;
								//LOGI("Has %d (max %d) of type: %d\n", typeCount, typeMax, currentMobType.mobClassId);
								if (typeCount >= typeMax)
									break;
							}
							
							maxCreatureCount = currentMobType.minCount + level->random.nextInt(1 + currentMobType.maxCount - currentMobType.minCount);
                        }

						Mob* tmp = MobFactory::getStaticTestMob(currentMobType.mobClassId, level);
						if (!tmp) continue;

						tmp->moveTo(xx, yy, zz, 0, 0);
						if (!tmp->canSpawn()) continue;

						Mob* mob = MobFactory::CreateMob(currentMobType.mobClassId, level);
						if (!mob) continue;

						if (addMob(level, mob, xx, yy, zz, level->random.nextFloat() * 360, 0, false)) {
							++currentCreatureCount;
							if (++clusterSize >= mob->getMaxSpawnClusterSize()) goto chunkLoop;
						}
						else
							delete mob;

                        count += clusterSize;
                    }
                }
            }
        }
    }
    return count;
}

/*static*/
void MobSpawner::postProcessSpawnMobs(Level* level, Biome* biome, int xo, int zo, int cellWidth, int cellHeight, Random* random) {

	//return;

    Biome::MobList mobs = biome->getMobs(MobCategory::creature);
    if (mobs.empty()) {
        return;
    }

    while (random->nextFloat() < biome->getCreatureProbability()) {

        Biome::MobSpawnerData* type = (Biome::MobSpawnerData*) WeighedRandom::getRandomItem(&level->random, mobs);
        int count = type->minCount + random->nextInt(1 + type->maxCount - type->minCount);

        int x = xo + random->nextInt(cellWidth);
        int z = zo + random->nextInt(cellHeight);
        int startX = x, startZ = z;

        for (int c = 0; c < count; c++) {
            bool success = false;
            for (int attempts = 0; !success && attempts < 4; attempts++) {
                // these mobs always spawn at the topmost position
                int y = level->getTopSolidBlock(x, z);
                if (isSpawnPositionOk(MobCategory::creature, level, x, y, z)) {

                    float xx = (float)x + 0.5f;
                    float yy = (float)y;
                    float zz = (float)z + 0.5f;

                    Mob* mob = MobFactory::CreateMob(type->mobClassId, level);
					if (!mob) continue;

                    // System.out.println("Placing night mob");
                    mob->moveTo(xx, yy, zz, random->nextFloat() * 360, 0);

                    level->addEntity(mob);
                    finalizeMobSettings(mob, level, xx, yy, zz);
                    success = true;
                }

                x += random->nextInt(5) - random->nextInt(5);
                z += random->nextInt(5) - random->nextInt(5);
                while (x < xo || x >= (xo + cellWidth) || z < zo || z >= (zo + cellWidth)) {
                    x = startX + random->nextInt(5) - random->nextInt(5);
                    z = startZ + random->nextInt(5) - random->nextInt(5);
                }
            }
        }
    }
}

/*static*/
TilePos MobSpawner::getRandomPosWithin(Level* level, int xo, int zo) {
    int x = xo + level->random.nextInt(16);
	int y = level->random.nextInt(Level::DEPTH); //@note: level->depth);
    int z = zo + level->random.nextInt(16);

    return TilePos(x, y, z);
}

/*static*/
bool MobSpawner::isSpawnPositionOk(const MobCategory& category, Level* level, int x, int y, int z) {
    if (category.getSpawnPositionMaterial() == Material::water) {
        return level->getMaterial(x, y, z)->isLiquid() && !level->isSolidBlockingTile(x, y + 1, z);
    } else {
        return level->isSolidBlockingTile(x, y - 1, z) && !level->isSolidBlockingTile(x, y, z) && !level->getMaterial(x, y, z)->isLiquid() && !level->isSolidBlockingTile(x, y + 1, z);
    }
}

/*static*/
void MobSpawner::finalizeMobSettings(Mob* mob, Level* level, float xx, float yy, float zz) {
	// @todo
//         if (mob instanceof Spider && level->random->nextInt(100) == 0) {
//             Skeleton skeleton = /*new*/ Skeleton(level);
//             skeleton.moveTo(xx, yy, zz, mob.yRot, 0);
//             level->addEntity(skeleton);
//             skeleton.ride(mob);
//         } else if (mob instanceof Sheep) {
//             ((Sheep) mob).setColor(Sheep.getSheepColor(level->random));
//         }

	if (mob->getEntityTypeId() == MobTypes::Sheep) {
		((Sheep*) mob)->setColor(Sheep::getSheepColor(&level->random));
	}

	makeBabyMob(mob, 0.5f);
}

/*static*/
bool MobSpawner::addMob(Level* level, Mob* mob, float xx, float yy, float zz, float yRot, float xRot, bool force)
{
	mob->moveTo(xx, yy, zz, yRot, xRot);

	if (force || mob->canSpawn()) {
		level->addEntity(mob);
		finalizeMobSettings(mob, level, xx, yy, zz);
		return true;
	} else {
		//LOGI("Couldn't add the entity\n");
		return false;
	}	
}

void MobSpawner::makeBabyMob( Mob* mob, float probability ) {
	static Random babyRandom(98495119L);
	if (MobTypes::BaseCreature == mob->getCreatureBaseType()) {
		if (babyRandom.nextFloat() < probability)
			((Animal*)mob)->setAge(-20 * 60 * SharedConstants::TicksPerSecond);
	}
}
