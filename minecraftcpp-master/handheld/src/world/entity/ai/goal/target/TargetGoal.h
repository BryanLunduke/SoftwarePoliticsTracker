#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__TargetGoal_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__TargetGoal_H__

//package net.minecraft.world.entity->ai.goal.target;

/* import net.minecraft.world.entity->* */
#include "../Goal.h"
/* import net.minecraft.world.entity->monster.* */
#include "../../../player/Player.h"

class Monster;

class TargetGoal: public Goal
{
public:
    static const int TargetFlag = 1;

    TargetGoal(Monster* mob, float within, bool mustSee)
    :   mob(mob),
        within(within),
        mustSee(mustSee)
    {}

    bool canContinueToUse() {
        Mob* target = mob->getTarget();
        if (target == NULL) return false;
        if (!target->isAlive()) return false;
        if (mob->distanceToSqr(target) > within * within) return false;
        if (mustSee && !mob->sensing->canSee(target)) return false;
        return true;
    }

    void stop() {
        mob->setTarget(NULL);
    }

protected:
    bool canAttack(Mob* target, bool allowInvulnerable) {
        if (target == NULL) return false;
        if (target == mob) return false;
        if (!target->isAlive()) return false;
        if (target->bb.y1 <= mob->bb.y0 || target->bb.y0 >= mob->bb.y1) return false;
        //if (target instanceof Creeper || target instanceof Ghast) return false;

        if (target->isPlayer()) {
            if (!allowInvulnerable && ((Player*) target)->abilities.invulnerable) return false;
        }
        if (mustSee && !mob->sensing->canSee(target)) return false;

        return true;
    }
    Monster* mob;
    float within;
    bool mustSee;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__TargetGoal_H__*/
