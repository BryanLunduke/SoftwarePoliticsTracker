#ifndef NET_MINECRAFT_WORLD_ENTITY__EntityEvent_H__
#define NET_MINECRAFT_WORLD_ENTITY__EntityEvent_H__

//package net.minecraft.world.entity;

class EntityEvent {
public:
    static const char JUMP = 1;
    static const char HURT = 2;
    static const char DEATH = 3;
    static const char START_ATTACKING = 4;
    static const char STOP_ATTACKING = 5;

    static const char TAMING_FAILED = 6;
    static const char TAMING_SUCCEEDED = 7;
    static const char SHAKE_WETNESS = 8;

    static const char USE_ITEM_COMPLETE = 9;

    static const char EAT_GRASS = 10;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY__EntityEvent_H__*/
