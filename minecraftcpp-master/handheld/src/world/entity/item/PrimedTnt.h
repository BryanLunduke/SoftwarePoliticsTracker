#ifndef NET_MINECRAFT_WORLD_ENTITY_ITEM__PrimedTnt_H__
#define NET_MINECRAFT_WORLD_ENTITY_ITEM__PrimedTnt_H__

//package net.minecraft.world.entity.item;

#include "../Entity.h"
#include "../../level/Level.h"

class CompoundTag;


class PrimedTnt: public Entity
{
	typedef Entity super;
public:
    PrimedTnt(Level* level);
    PrimedTnt(Level* level, float x, float y, float z);

	void tick();
	
	bool isPickable();

	int getEntityTypeId() const;

	float getShadowHeightOffs();
protected:
    void addAdditonalSaveData(CompoundTag* entityTag);
	void readAdditionalSaveData(CompoundTag* tag);
private:
    void explode();

public:
	int life;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ITEM__PrimedTnt_H__*/
