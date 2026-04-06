#ifndef NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Cow_H__
#define NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Cow_H__

//package net.minecraft.world.entity.animal;

#include "Animal.h"

class Level;
class Player;
class CompoundTag;

class Cow: public Animal
{
	typedef Animal super;
public:
    Cow(Level* level);

	int getEntityTypeId() const;

    /*@Override*/
    int getMaxHealth();

    void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

    bool interact(Player* player);
protected:
    const char* getAmbientSound();
    std::string getHurtSound();
    std::string getDeathSound();

    float getSoundVolume();

    int getDeathLoot();
    /*@Override*/
    void dropDeathLoot(/*bool wasKilledByPlayer, int playerBonusLevel*/);

    /*@Override*/
    Animal* getBreedOffspring(Animal* target);
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ANIMAL__Cow_H__*/
