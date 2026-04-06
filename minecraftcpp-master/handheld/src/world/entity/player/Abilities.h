#ifndef NET_MINECRAFT_WORLD_ENTITY_PLAYER__Abilities_H__
#define NET_MINECRAFT_WORLD_ENTITY_PLAYER__Abilities_H__

//package net.minecraft.world.entity.player;

#include "../../../nbt/CompoundTag.h"

class Abilities
{
public:
    Abilities()
    :   invulnerable(false),
        flying(false),
        mayfly(false),
        instabuild(false)
    {
	}

    void addSaveData(CompoundTag* parentTag) {
        CompoundTag* tag = new CompoundTag();

        tag->putBoolean("invulnerable", invulnerable);
        tag->putBoolean("flying", invulnerable);
        tag->putBoolean("mayfly", mayfly);
        tag->putBoolean("instabuild", instabuild);

        parentTag->put("abilities", tag);
    }

    void loadSaveData(CompoundTag* parentTag) {
        if (parentTag->contains("abilities")) {
            CompoundTag* tag = parentTag->getCompound("abilities");

            invulnerable = tag->getBoolean("invulnerable");
            flying = tag->getBoolean("flying");
            mayfly = tag->getBoolean("mayfly");
            instabuild = tag->getBoolean("instabuild");
        }
    }

    bool invulnerable;
    bool flying;
    bool mayfly;
    bool instabuild;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_PLAYER__Abilities_H__*/
