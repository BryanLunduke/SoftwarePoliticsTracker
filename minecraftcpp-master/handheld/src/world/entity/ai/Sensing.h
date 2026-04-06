#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_SENSING__Sensing_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_SENSING__Sensing_H__

//package net.minecraft.world.entity.ai.sensing;

#include <set>

class Sensing {
    typedef std::set<Entity*> EntitySet;
public:
    Sensing(Mob* mob)
    :   mob(mob)
    {}

    void tick() {
        seen.clear();
        unseen.clear();
    }

    bool canSee(Entity* target) {
        EntitySet::const_iterator cSeen = seen.find(target);
        if (cSeen != seen.end()) return true;

        EntitySet::const_iterator cUnseen = unseen.find(target);
        if (cUnseen != unseen.end()) return false;

        bool canSee = mob->canSee(target);
        if (canSee) seen.insert(target);
        else unseen.insert(target);
        return canSee;
    }
private:
    Mob* mob;
    EntitySet seen;
    EntitySet unseen;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_SENSING__Sensing_H__*/
