#ifndef NET_MINECRAFT_WORLD_ENTITY_ITEM__ItemEntity_H__
#define NET_MINECRAFT_WORLD_ENTITY_ITEM__ItemEntity_H__

//package net.minecraft.world.entity.item;

#include "../../item/ItemInstance.h"
#include "../../entity/Entity.h"
#include "../../../SharedConstants.h"

class Level;
class ItemInstance;

class ItemEntity: public Entity
{
	typedef Entity super;

    static const int LIFETIME;
public:
	ItemEntity(Level* level);
	ItemEntity(Level* level, float x, float y, float z, const ItemInstance& item);
	~ItemEntity();

    void tick();
    bool isInWater();
    bool hurt(Entity* source, int damage);
    void playerTouch(Player* player);
	bool isItemEntity();
	int getEntityTypeId() const;
	int getLifeTime() const;
protected:
	void burn(int dmg);

	void addAdditonalSaveData(CompoundTag* entityTag);
	void readAdditionalSaveData(CompoundTag* tag);
private:
    bool checkInTile(float x, float y, float z);

public:
	ItemInstance item;
	int age;
	int throwTime;
	float bobOffs;
private:
	int tickCount;
	int health;
	int lifeTime;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ITEM__ItemEntity_H__*/
