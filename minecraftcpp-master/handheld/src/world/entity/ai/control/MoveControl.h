#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__MoveControl_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__MoveControl_H__

//package net.minecraft.world.entity.ai.control;

#include "Control.h"

#include "../../Mob.h"
#include "../../../../util/Mth.h"

class MoveControl: public Control
{
    static const float MAX_TURN;
public:
    static const float MIN_SPEED;
    static const float MIN_SPEED_SQR;

    MoveControl(Mob* mob);

    bool hasWanted();

    float getSpeed();

    void setWantedPosition(float x, float y, float z, float speed);

    void tick();
private:
    float rotlerp(float a, float b, float max);

    Mob* mob;
    float wantedX;
    float wantedY;
    float wantedZ;
    float speed;
    bool _hasWanted;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__MoveControl_H__*/
