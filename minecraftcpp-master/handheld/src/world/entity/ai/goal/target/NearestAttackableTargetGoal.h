#ifndef NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__NearestAttackableTargetGoal_H__
#define NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__NearestAttackableTargetGoal_H__

//package net.minecraft.world.entity->ai.goal.target;

/* import net.minecraft.world.entity->* */
#include "TargetGoal.h"
#include "../../../player/Player.h"


class NearestAttackableTargetGoal: public TargetGoal
{
	typedef TargetGoal super;
public:
    NearestAttackableTargetGoal(Monster* mob, int targetType, float within, int randomInterval, bool mustSee)
    :   super(mob, within, mustSee),
        targetType(targetType),
        randomInterval(randomInterval)
    {
        setRequiredControlFlags(TargetGoal::TargetFlag);
    }

    bool canUse() {
        if (randomInterval > 0 && mob->random.nextInt(randomInterval) != 0) return false;
        Mob* potentialTarget = NULL;
        if (targetType == 1) potentialTarget = mob->level->getNearestPlayer(mob, within); //@todo: targetType
        //else potentialTarget = (Mob*) mob->level->getClosestEntityOfClass(targetType, mob->bb.grow(within, 4, within), mob);
        if (!canAttack(potentialTarget, false)) return false;
        target = potentialTarget;
        return true;
    }

    void start() {
        mob->setTarget(target);
    }
private:
    Mob* target;
    int  targetType;
    int  randomInterval;
};

#endif /*NET_MINECRAFT_WORLD_ENTITY_AI_GOAL_TARGET__NearestAttackableTargetGoal_H__*/
