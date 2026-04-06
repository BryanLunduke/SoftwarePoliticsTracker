//package net.minecraft.world.entity;

#include "../../platform/log.h"

class Material;

class MobCategory
{
public:
	static void initMobCategories();

    //
    static const MobCategory monster;
    static const MobCategory creature;
    static const MobCategory waterCreature;
	//@todo: rewrite to std::vector with [] init
	static const MobCategory* const values[];
	static const int numValues;

    int getBaseClassId() const {
        return _baseClassId;
    }

    int getMaxInstancesPerChunk() const {
        return _max;
    }

	int getMaxInstancesPerLevel() const {
		return _maxPerLevel;
	}

    const Material* getSpawnPositionMaterial() const {
        return _spawnPositionMaterial;
    }

    bool isFriendly() const {
        return _isFriendly;
    }

private:
    const int _baseClassId;
    const int _max;
	const int _maxPerLevel;
    mutable const Material* _spawnPositionMaterial;
    const bool _isFriendly;

    MobCategory(int baseClassId, int max, int maxPerLevel, bool isFriendly)
    :   _baseClassId(baseClassId),
        _max(max),
		_maxPerLevel(maxPerLevel),
		_spawnPositionMaterial(NULL),
        _isFriendly(isFriendly)
    {
		//LOGI("Creating a Mobcategory: %d, %d, %p, %d    ", _baseClassId, _max, _spawnPositionMaterial, _isFriendly);
	}

	void setMaterial(const Material* material) const {
		_spawnPositionMaterial = material;
	}
};
