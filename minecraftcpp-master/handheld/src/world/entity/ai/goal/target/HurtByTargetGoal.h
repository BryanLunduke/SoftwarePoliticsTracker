#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__HurtByTargetGoal_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__HurtByTargetGoal_H__

//package net.minecraft.world.entity->ai.goal.target;

#include "TargetGoal.h"
#include "../../../../phys/AABB.h"

class HurtByTargetGoal: public TargetGoal
{
    typedef TargetGoal super;
public:
    HurtByTargetGoal(Monster* mob, bool alertSameType)
    :   super(mob, 16, false),
        alertSameType(alertSameType)
    {
        setRequiredControlFlags(TargetGoal::TargetFlag);
    }

    bool canUse() {
        return canAttack(mob->getLastHurtByMob(), true);
    }

    void start() {
        mob->setTarget(mob->getLastHurtByMob());

//         if (alertSameType) {
//             List<Entity> nearby = mob.level->getEntitiesOfClass(mob.getClass(), AABB.newTemp(mob.x, mob.y, mob.z, mob.x + 1, mob.y + 1, mob.z + 1).grow(within, 4, within));
//             for (Entity* ent : nearby) {
//                 Mob other = (Mob) ent;
//                 if (this->mob == other) continue;
//                 if (other.getTarget() != NULL) continue;
//                 other.setTarget(mob.getLastHurtByMob());
//             }
//         }
    }
private:
	bool alertSameType;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__HurtByTargetGoal_H__*/
