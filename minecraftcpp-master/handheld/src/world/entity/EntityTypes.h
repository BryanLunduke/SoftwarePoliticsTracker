#ifndef NET_MINECRAFT_WORLD_ENTITY__EntityTypes_H__
#define NET_MINECRAFT_WORLD_ENTITY__EntityTypes_H__

#include "../../util/Mth.h"

//? 3 bits ?
class BaseTypes {
public:
	static const int Entity = 1;
	static const int Item   = 2;
};

//? 4 bits ?
class EntityTypes {
public:
	/*
	static const int Mob		= 1;
	static const int Particle	= 2;
	static const int ItemEntity = 3;
	*/

	// Forgive me, but I'm not sure I have time to finish my RTTI
	// implementation -> this. Mobs up to 63, rest on top of that
	static const int IdItemEntity = 64;

	static const int IdPrimedTnt = 65;
    static const int IdFallingTile = 66;

	static const int IdArrow	= 80;
	static const int IdSnowball	= 81;
	static const int IdThrownEgg= 82;
	static const int IdPainting = 83;
};

// For now; mobs 0-63
// Those constants are unfortunately the same "type" as EntityTypes
// @todo: fix something nicer (but keep the IDs)
class MobTypes {
public:
    static const int BaseEnemy			= 1;
    static const int BaseCreature		= 2;
    static const int BaseWaterCreature	= 3;

	static const int Chicken    = 10;
	static const int Cow        = 11;
    static const int Pig        = 12;
	static const int Sheep      = 13;

	static const int Zombie		= 32;
	static const int Creeper	= 33;
	static const int Skeleton	= 34;
	static const int Spider		= 35;
	static const int PigZombie	= 36;
};

struct ParticleType {
	enum Id {
		none,
		bubble,
		crit,
		smoke,
		explode,
		flame,
		lava,
		largesmoke,
		reddust,
		iconcrack,
		snowballpoof,
		largeexplode,
		hugeexplosion
	};
};

#define PARTICLETYPE(x) (ParticleType::x,#x)

//
//
//
/*
class ClassTree {
public:
	static bool isEntityType(int type, int entityType) {
		return getBaseType(type) == BaseTypes::Entity? getSubType(1, type) == entityType : false;
	}
	static bool isBaseType(int type, int baseType) {
		return getBaseType(type) == getBaseType(baseType);
	}
	static int getBaseType(int type) {
		return getSubType(0, type);
	}
	static int getSubType(int subTypeIndex, int type) {
		return type & (0xff << (subTypeIndex << 3));
	}
	static bool isSameTypeUpTo(int a, int b, int hierarchy) {
		if (hierarchy < 0) hierarchy = Mth::Min(getHierarchyLevel(a), getHierarchyLevel(b));
		for (int i = 0; i <= hierarchy; ++i)
			if (getSubType(i, a) != getSubType(i, b)) return false;
		return true;
	}
	static int getHierarchyLevel(int type) {
		if (type & 0xff) {
			if (type & 0xff00) {
				if (type & 0xff0000) {
					if (type & 0xff000000) return 3;
					return 2;
				}
				return 1;
			}
			return 0;
		}
		return -1;
	}
	static int createBaseType(int type) {
		return _create(type);
	}
	static int createEntityType(int type) {
		return _create(BaseTypes::Entity, type);
	}
	static int createMobType(int type) {
		return _create(BaseTypes::Entity, EntityTypes::Mob, type);
	}
	static int _create(int base, int a = 0, int b = 0, int c = 0) {
		return base | (a << 8) | (b << 16) | (c << 24);
	}
};

*/

#endif /*NET_MINECRAFT_WORLD_ENTITY__EntityTypes_H__*/
