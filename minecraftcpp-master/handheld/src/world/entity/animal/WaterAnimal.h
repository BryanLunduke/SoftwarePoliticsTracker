#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__WaterAnimal_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__WaterAnimal_H__

//package net.minecraft.world.entity.animal;

#include "../PathfinderMob.h"
#include "../Creature.h"

class Level;
class Player;
class CompoundTag;

/**
 * The purpose of this class is to offer a new base class for MobCategory.
 * {@link MobCategory}
 *
 * Note: Can't extend Animal because then water animals would prevent normal
 * animals to spawn, due to MobCategory.getMaxInstancesPerChunk(). This class is
 * otherwise similar to Animal
 *
 */
class WaterAnimal: public PathfinderMob, public Creature
{
    typedef PathfinderMob super;
public:
    WaterAnimal(Level* level);

    /*@Override*/
    bool isWaterMob();

    void addAdditonalSaveData(CompoundTag* entityTag);
    void readAdditionalSaveData(CompoundTag* tag);

    bool canSpawn();

    int getAmbientSoundInterval();
	int getCreatureBaseType() const;

protected:
    bool removeWhenFarAway();

    /*@Override*/
    int getExperienceReward(Player* killedBy);
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ANIMAL__WaterAnimal_H__*/
