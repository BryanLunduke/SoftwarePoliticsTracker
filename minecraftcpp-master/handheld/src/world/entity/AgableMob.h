#ifndef NET_MINECRAFT_WORLD_ENTITY__AgableMob_H__
#define NET_MINECRAFT_WORLD_ENTITY__AgableMob_H__

#include "PathfinderMob.h"

//package net.minecraft.world.entity;

class AgableMob: public PathfinderMob
{
    typedef PathfinderMob super;
public:
    AgableMob(Level* level);

    int getAge();
    void setAge(int age);
    bool isBaby();

    void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

    void aiStep();
private:
    int age;

	static const int DATA_FLAGS_ID = 14;
	// Flags values are bit shifted
	static const int DATAFLAG_ISBABY = 0;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__AgableMob_H__*/
