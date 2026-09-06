#ifndef NET_MINECRAFT_WORLD_ENTITY__MobFactory_H__
#define NET_MINECRAFT_WORLD_ENTITY__MobFactory_H__

#include "EntityTypes.h"
#include "animal/AnimalInclude.h"
#include "monster/MonsterInclude.h"

class MobFactory {
public:
    static Mob* CreateMob(int mobType, Level* level) {
		//LOGI("Trying to create a mob with type: %d!\n", mobType);

		Mob* mob = NULL;
		switch(mobType) {
			// Animals
		case MobTypes::Chicken:
			mob = new Chicken(level);
			break;
		case MobTypes::Cow:
			mob = new Cow(level);
			break;
			case MobTypes::Pig:
				mob = new Pig(level);
				break;
			case MobTypes::Sheep:
				mob = new Sheep(level);
				break;

			// Monsters
			case MobTypes::Creeper:
				mob = new Creeper(level);
				break;
			case MobTypes::Zombie:
				mob = new Zombie(level);
				break;
			case MobTypes::Skeleton:
				mob = new Skeleton(level);
				break;
			case MobTypes::Spider:
				mob = new Spider(level);
				break;
			case MobTypes::PigZombie:
				mob = new PigZombie(level);
				break;
			default:
                LOGE("Unknown mob type requested: %d\n", mobType);
                break;
        }

		if (mob) {
			mob->health = mob->getMaxHealth();
		}
		return mob;
    }

	static void clearStaticTestMobs() {
		getStaticTestMob(0, NULL);
	}

	// @huge @attn: Those needs to be cleared for every new level
	static Mob* getStaticTestMob(int mobType, Level* level) {
		static std::map<int, Mob*> _mobs;
		static Level* lastLevel = NULL;

		bool wantClear = (mobType == 0) && (level == NULL);
		bool newLevel  = (level != lastLevel);
		lastLevel = level;

		// We either want to clear all mobs, or a new level is created
		if (wantClear || newLevel) {
			for (std::map<int, Mob*>::iterator it = _mobs.begin(); it != _mobs.end(); ++it)
				delete it->second;
			_mobs.clear();

			if (wantClear) return NULL;
		}

		std::map<int, Mob*>::iterator it = _mobs.find(mobType);

		if (it != _mobs.end())
			return it->second;

		// Didn't exist, add it
		Mob* mob = CreateMob(mobType, level);
		_mobs.insert(std::make_pair(mobType, mob));
		return mob;
	}
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__MobFactory_H__*/
