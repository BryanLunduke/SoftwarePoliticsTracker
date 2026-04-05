#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Zombie_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Zombie_H__

//package net.minecraft.world.entity->monster;

#include "Monster.h"
#include <string>

class Level;

class Zombie: public Monster
{
	typedef Monster super;
public:
    Zombie(Level* level);

	~Zombie();

    /*@Override*/
    int getMaxHealth();

    void aiStep();
	virtual int getEntityTypeId() const;
	void setUseNewAi(bool use);
	virtual void die(Entity* source);
	virtual int getAttackDamage(Entity* target);
protected:
	/*@Override*/
	int getArmorValue();

	const char* getAmbientSound();
    std::string getHurtSound();
    std::string getDeathSound();
	
	//@todo
    int getDeathLoot();

	virtual bool useNewAi();

	int fireCheckTick;
	bool _useNewAi;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_MONSTER__Zombie_H__*/
