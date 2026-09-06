#ifndef NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Arrow_H__
#define NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Arrow_H__

//package net.minecraft.world.entity.projectile;

#include "../Entity.h"

class Level;
class Mob;
class CompoundTag;

class Arrow: public Entity
{
    typedef Entity super;
    // base damage, multiplied with velocity
    static const float ARROW_BASE_DAMAGE;

public:
    Arrow(Level* level);
    Arrow(Level* level, float x, float y, float z);
    Arrow(Level* level, Mob* mob, float power);

    void _init();

    void shoot(float xd, float yd, float zd, float pow, float uncertainty);
    
	void lerpMotion(float xd, float yd, float zd);

	void tick();

	int getEntityTypeId() const;

    void addAdditonalSaveData(CompoundTag* tag);
    void readAdditionalSaveData(CompoundTag* tag);

    void playerTouch(Player* player);

    float getShadowHeightOffs();

	int getAuxData();
public:
    bool playerArrow;
    int shakeTime;
    int ownerId;
    bool critArrow;
private:
    int xTile;
    int yTile;
    int zTile;
    int lastTile;
    int lastData;
    bool inGround;
    int life;
    int flightTime;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PROJECTILE__Arrow_H__*/
