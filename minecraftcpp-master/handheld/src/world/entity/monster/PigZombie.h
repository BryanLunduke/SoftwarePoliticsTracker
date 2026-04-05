#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__PigZombie_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__PigZombie_H__

#include "Zombie.h"
class ItemInstance;
class PigZombie : public Zombie {
	typedef Zombie super;
public:
	PigZombie(Level* level);
	bool useNewAi();
	void tick();
	std::string getTexture();
	bool canSpawn();
	void addAdditonalSaveData(CompoundTag* entityTag);
	void readAdditionalSaveData(CompoundTag* tag);
	bool hurt(Entity* sourceEntity, int dmg);
	bool interact(Player* player);
	int getEntityTypeId() const;
	virtual int getAttackTime();
	ItemInstance* getCarriedItem();
protected:
	Entity* findAttackTarget();
	const char* getAmbientSound();
	std::string getHurtSound();
	std::string getDeathSound();
	void dropDeathLoot();
	int getDeathLoot();
private:
	void alert(Entity* target);
	
private:
	int angerTime;
	int playAngrySoundIn;
	int stunedTime;
	ItemInstance weapon;
};

#endif /* NET_MINECRAFT_WORLD_ENTITY_MONSTER__PigZombie_H__ */