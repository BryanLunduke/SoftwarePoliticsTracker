#ifndef NET_MINECRAFT_WORLD_ENTITY_ITEM__FallingTile_H__
#define NET_MINECRAFT_WORLD_ENTITY_ITEM__FallingTile_H__

//package net.minecraft.world.entity.item;

#include "../Entity.h"

class Level;
class CompoundTag;

class FallingTile: public Entity
{
public:
    FallingTile(Level* level);
    FallingTile(Level* level, float x, float y, float z, int tile, int data);

    void init();

    bool isPickable();

    void tick();

	int getEntityTypeId() const;

	float getShadowHeightOffs();

    Level* getLevel();

protected:
	void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

public:
	int tile;
    int data;

    int time;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_ITEM__FallingTile_H__*/
