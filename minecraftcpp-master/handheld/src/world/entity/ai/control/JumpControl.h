#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__JumpControl_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__JumpControl_H__

//package net.minecraft.world.entity.ai.control;

#include "Control.h"
#include "../../Mob.h"

class JumpControl: public Control
{
public:
    JumpControl(Mob* mob)
    :   mob(mob),
        _jump(false)
    {
    }

    void jump() {
        _jump = true;
    }

    void tick() {
        mob->setJumping(_jump);
        _jump = false;
    }
private:
    Mob* mob;
    bool _jump;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_CONTROL__JumpControl_H__*/
